#include "server/pch.hh"

#include "server/world/zcol_file.hh"

#include "core/buffer.hh"
#include "core/utils/crc64.hh"

constexpr static std::uint8_t ZCOL_MAGIC_1 = 0x5A;
constexpr static std::uint8_t ZCOL_MAGIC_2 = 0x43;
constexpr static std::uint8_t ZCOL_MAGIC_3 = 0x4F;
constexpr static std::uint8_t ZCOL_MAGIC_4 = 0x4C;
constexpr static std::uint32_t ZCOL_VERSION = 0x00000001;
constexpr static std::size_t TABLE_BYTES = constant::REGION_AREA * sizeof(std::uint64_t);

ZcolFile::ZcolFile(std::filesystem::path path) : m_path(std::move(path))
{
    m_offsets.fill(0);
    m_lengths.fill(0);
    m_checksums.fill(0);

    std::error_code ec;
    std::filesystem::create_directories(m_path.parent_path(), ec);

    if(std::filesystem::exists(m_path)) {
        m_stream.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
        m_valid = m_stream.is_open();
        m_valid = m_valid && load_header();

        if(!m_valid) {
            // The generator can rebuild a slice, so an unknown
            // header means an older format and the file is rewritten
            m_stream.close();
            m_stream.open(m_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
            m_valid = m_stream.is_open();

            if(m_valid) {
                m_offsets.fill(0);
                m_lengths.fill(0);
                m_checksums.fill(0);
                save_header();
            }
        }
        else {
            build_free_ranges();
        }
    }
    else {
        m_stream.open(m_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        m_valid = m_stream.is_open();

        if(m_valid) {
            save_header();
        }
    }
}

ZcolFile::~ZcolFile(void)
{
    if(m_stream.is_open()) {
        m_stream.flush();
        m_stream.close();
    }
}

bool ZcolFile::is_valid(void) const
{
    return m_valid;
}

bool ZcolFile::read_slot(std::size_t slot, ReadBuffer& buffer)
{
    if(!m_valid || slot >= constant::REGION_AREA) {
        return false;
    }

    auto offset = m_offsets[slot];
    auto length = m_lengths[slot];
    auto checksum = m_checksums[slot];

    if(offset == 0 || length == 0) {
        return false;
    }

    std::vector<std::byte> payload;
    payload.resize(length);

    m_stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    m_stream.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));

    if(!m_stream.good()) {
        return false;
    }

    auto payload_checksum = utils::crc64(payload);

    if(payload_checksum == checksum) {
        buffer.reset(payload);
        return true;
    }

    return false;
}

void ZcolFile::write_slot(std::size_t slot, const WriteBuffer& buffer)
{
    if(!m_valid || slot >= constant::REGION_AREA) {
        return;
    }

    auto data = buffer.data();
    auto size = buffer.size();
    auto length = static_cast<std::uint64_t>(size);
    auto checksum = utils::crc64(std::span<const std::byte>(data, size));

    if(m_lengths[slot]) {
        insert_free_range(m_offsets[slot], m_lengths[slot]);
        m_offsets[slot] = 0;
        m_lengths[slot] = 0;
    }

    std::uint64_t offset;

    if(auto reused = take_free_range(length)) {
        offset = reused.value();
        m_stream.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    }
    else {
        m_stream.seekp(0, std::ios::end);
        auto end_position = m_stream.tellp();
        offset = static_cast<std::uint64_t>(end_position);
    }

    m_stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));

    if(!m_stream.good()) {
        return;
    }

    m_offsets[slot] = offset;
    m_lengths[slot] = length;
    m_checksums[slot] = checksum;

    save_header();
}

std::size_t ZcolFile::header_size(void)
{
    std::size_t result = 0;
    result += sizeof(ZCOL_MAGIC_1);
    result += sizeof(ZCOL_MAGIC_2);
    result += sizeof(ZCOL_MAGIC_3);
    result += sizeof(ZCOL_MAGIC_4);
    result += sizeof(ZCOL_VERSION);
    result += TABLE_BYTES;
    result += TABLE_BYTES;
    result += TABLE_BYTES;
    return result;
}

bool ZcolFile::load_header(void)
{
    auto size = header_size();

    char gambit_character;
    m_stream.seekg(static_cast<std::streamoff>(size - 1), std::ios::beg);
    m_stream.read(&gambit_character, sizeof(gambit_character));

    if(!m_stream.good()) {
        return false;
    }

    std::vector<std::byte> header;
    header.resize(size);

    m_stream.seekg(0, std::ios::beg);
    m_stream.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));

    if(!m_stream.good()) {
        return false;
    }

    ReadBuffer buffer;
    buffer.reset(header);

    auto magic_1 = buffer.read<std::uint8_t>();
    auto magic_2 = buffer.read<std::uint8_t>();
    auto magic_3 = buffer.read<std::uint8_t>();
    auto magic_4 = buffer.read<std::uint8_t>();
    auto version = buffer.read<std::uint32_t>();

    auto header_valid = true;
    header_valid = header_valid && magic_1 == ZCOL_MAGIC_1;
    header_valid = header_valid && magic_2 == ZCOL_MAGIC_2;
    header_valid = header_valid && magic_3 == ZCOL_MAGIC_3;
    header_valid = header_valid && magic_4 == ZCOL_MAGIC_4;
    header_valid = header_valid && version <= ZCOL_VERSION;

    if(!header_valid) {
        return false;
    }

    for(std::size_t i = 0; i < constant::REGION_AREA; ++i) {
        m_offsets[i] = buffer.read<std::uint64_t>();
        m_lengths[i] = buffer.read<std::uint64_t>();
        m_checksums[i] = buffer.read<std::uint64_t>();
    }

    return true;
}

void ZcolFile::save_header(void)
{
    WriteBuffer buffer;

    buffer.write<std::uint8_t>(ZCOL_MAGIC_1);
    buffer.write<std::uint8_t>(ZCOL_MAGIC_2);
    buffer.write<std::uint8_t>(ZCOL_MAGIC_3);
    buffer.write<std::uint8_t>(ZCOL_MAGIC_4);
    buffer.write<std::uint32_t>(ZCOL_VERSION);

    for(std::size_t i = 0; i < constant::REGION_AREA; ++i) {
        buffer.write<std::uint64_t>(m_offsets[i]);
        buffer.write<std::uint64_t>(m_lengths[i]);
        buffer.write<std::uint64_t>(m_checksums[i]);
    }

    m_stream.seekp(0, std::ios::beg);
    buffer.to_stream(m_stream);
}

void ZcolFile::build_free_ranges(void)
{
    m_free_ranges.clear();

    std::vector<std::pair<std::uint64_t, std::uint64_t>> used;

    for(std::size_t i = 0; i < constant::REGION_AREA; ++i) {
        if(m_lengths[i]) {
            used.emplace_back(m_offsets[i], m_lengths[i]);
        }
    }

    std::sort(used.begin(), used.end());

    std::uint64_t cursor = static_cast<std::uint64_t>(header_size());

    for(const auto& [offset, length] : used) {
        if(offset > cursor) {
            m_free_ranges.emplace_back(cursor, offset - cursor);
        }

        cursor = std::max(cursor, offset + length);
    }
}

std::optional<std::uint64_t> ZcolFile::take_free_range(std::uint64_t size)
{
    std::optional<std::size_t> best;

    for(std::size_t i = 0; i < m_free_ranges.size(); ++i) {
        if(m_free_ranges[i].second < size) {
            continue;
        }

        if(!best.has_value() || m_free_ranges[i].second < m_free_ranges[best.value()].second) {
            best = i;
        }
    }

    if(!best.has_value()) {
        return std::nullopt;
    }

    auto& range = m_free_ranges[best.value()];
    auto offset = range.first;

    if(range.second == size) {
        m_free_ranges.erase(m_free_ranges.begin() + static_cast<std::ptrdiff_t>(best.value()));
    }
    else {
        range.first += size;
        range.second -= size;
    }

    return offset;
}

void ZcolFile::insert_free_range(std::uint64_t offset, std::uint64_t length)
{
    if(length == 0) {
        return;
    }

    auto end = offset + length;
    auto it = std::lower_bound(m_free_ranges.begin(), m_free_ranges.end(), offset, [](const auto& range, auto value) {
        return range.first < value;
    });

    if(it != m_free_ranges.end() && end == it->first) {
        length += it->second;
        it = m_free_ranges.erase(it);
    }

    if(it != m_free_ranges.begin()) {
        auto prev = std::prev(it);

        if(prev->first + prev->second == offset) {
            offset = prev->first;
            length += prev->second;
            it = m_free_ranges.erase(prev);
        }
    }

    m_free_ranges.insert(it, std::make_pair(offset, length));
}
