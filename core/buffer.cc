#include "core/pch.hh"

#include "core/buffer.hh"

constexpr static std::uint8_t COMPRESSION_NONE = 0;
constexpr static std::uint8_t COMPRESSION_ZLIB = 1;

ReadBuffer::ReadBuffer(const ReadBuffer& other)
{
    reset(std::span(other.data(), other.size()));
}

ReadBuffer::ReadBuffer(std::span<const std::byte> data)
{
    reset(data);
}

ReadBuffer::ReadBuffer(const ENetPacket* packet)
{
    assert(packet);

    reset(packet);
}

ReadBuffer::ReadBuffer(PHYSFS_File* file)
{
    assert(file);

    reset(file);
}

std::size_t ReadBuffer::size(void) const
{
    return m_vector.size();
}

const std::byte* ReadBuffer::data(void) const
{
    return m_vector.data();
}

void ReadBuffer::reset(std::span<const std::byte> data)
{
    m_vector.assign(data.begin(), data.end());
    m_position = 0;
}

void ReadBuffer::reset(const ENetPacket* packet)
{
    assert(packet);

    auto bytes_ptr = reinterpret_cast<const std::byte*>(packet->data);
    m_vector.assign(bytes_ptr, bytes_ptr + packet->dataLength);
    m_position = 0;
}

void ReadBuffer::reset(PHYSFS_File* file)
{
    assert(file);

    auto size = PHYSFS_fileLength(file);
    assert(size > 0);

    m_vector.resize(static_cast<std::size_t>(size));
    m_position = 0;

    PHYSFS_seek(file, 0);
    PHYSFS_readBytes(file, m_vector.data(), m_vector.size());
}

void ReadBuffer::reset(std::istream& stream)
{
    assert(stream.good());

    stream.seekg(0, std::ios::end);
    auto size = stream.tellg();
    assert(size > 0);

    m_vector.resize(static_cast<std::size_t>(size));
    m_position = 0;

    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(m_vector.data()), m_vector.size());
}

bool ReadBuffer::has_records(std::uint32_t count, std::size_t record_size) const
{
    auto total_size = record_size * static_cast<std::uint64_t>(count);
    return total_size + m_position <= m_vector.size();
}

void ReadBuffer::seek(std::size_t position)
{
    assert(position <= m_vector.size());

    m_position = position;
}

template<>
std::byte ReadBuffer::read<std::byte>(void)
{
    if(m_position < m_vector.size()) {
        auto result = m_vector[m_position];
        m_position += 1;
        return result;
    }

    m_position += 1;
    return static_cast<std::byte>(0x00);
}

template<>
std::uint8_t ReadBuffer::read<std::uint8_t>(void)
{
    if((m_position + 1) <= m_vector.size()) {
        auto result = static_cast<std::uint8_t>(m_vector[m_position]);
        m_position += 1;
        return result;
    }

    m_position += 1;
    return 0;
}

template<>
std::uint16_t ReadBuffer::read<std::uint16_t>(void)
{
    std::uint16_t result = 0;

    if((m_position + sizeof(result)) <= m_vector.size()) {
        std::memcpy(&result, &m_vector[m_position], sizeof(result));

        if constexpr(std::endian::native == std::endian::big) {
            result = std::byteswap(result);
        }
    }

    m_position += sizeof(result);
    return result;
}

template<>
std::uint32_t ReadBuffer::read<std::uint32_t>(void)
{
    std::uint32_t result = 0;

    if((m_position + sizeof(result)) <= m_vector.size()) {
        std::memcpy(&result, &m_vector[m_position], sizeof(result));

        if constexpr(std::endian::native == std::endian::big) {
            result = std::byteswap(result);
        }
    }

    m_position += sizeof(result);
    return result;
}

template<>
std::uint64_t ReadBuffer::read<std::uint64_t>(void)
{
    std::uint64_t result = 0;

    if((m_position + sizeof(result)) <= m_vector.size()) {
        std::memcpy(&result, &m_vector[m_position], sizeof(result));

        if constexpr(std::endian::native == std::endian::big) {
            result = std::byteswap(result);
        }
    }

    m_position += sizeof(result);
    return result;
}

template<>
float ReadBuffer::read<float>(void)
{
    return std::bit_cast<float>(read<std::uint32_t>());
}

template<>
std::int8_t ReadBuffer::read<std::int8_t>(void)
{
    return std::bit_cast<std::int8_t>(read<std::uint8_t>());
}

template<>
std::int16_t ReadBuffer::read<std::int16_t>(void)
{
    return std::bit_cast<std::int16_t>(read<std::uint16_t>());
}

template<>
std::int32_t ReadBuffer::read<std::int32_t>(void)
{
    return std::bit_cast<std::int32_t>(read<std::uint32_t>());
}

template<>
std::int64_t ReadBuffer::read<std::int64_t>(void)
{
    return std::bit_cast<std::int64_t>(read<std::uint64_t>());
}

template<>
std::string ReadBuffer::read<std::string>(void)
{
    std::string result;
    result.resize(read<std::uint16_t>());

    for(std::size_t i = 0; i < result.size(); ++i) {
        if(m_position < m_vector.size()) {
            result[i] = static_cast<char>(m_vector[m_position]);
        }

        m_position += 1;
    }

    return result;
}

template<typename T, std::size_t N>
Eigen::Vector<T, N> ReadBuffer::read_vector(void)
{
    Eigen::Vector<T, N> result;

    for(std::size_t i = 0; i < N; ++i) {
        result[i] = read<T>();
    }

    return result;
}

template Eigen::Vector<std::uint8_t, 2> ReadBuffer::read_vector<std::uint8_t, 2>(void);
template Eigen::Vector<std::uint8_t, 3> ReadBuffer::read_vector<std::uint8_t, 3>(void);
template Eigen::Vector<std::uint8_t, 4> ReadBuffer::read_vector<std::uint8_t, 4>(void);

template Eigen::Vector<std::uint16_t, 2> ReadBuffer::read_vector<std::uint16_t, 2>(void);
template Eigen::Vector<std::uint16_t, 3> ReadBuffer::read_vector<std::uint16_t, 3>(void);
template Eigen::Vector<std::uint16_t, 4> ReadBuffer::read_vector<std::uint16_t, 4>(void);

template Eigen::Vector<std::uint32_t, 2> ReadBuffer::read_vector<std::uint32_t, 2>(void);
template Eigen::Vector<std::uint32_t, 3> ReadBuffer::read_vector<std::uint32_t, 3>(void);
template Eigen::Vector<std::uint32_t, 4> ReadBuffer::read_vector<std::uint32_t, 4>(void);

template Eigen::Vector<std::uint64_t, 2> ReadBuffer::read_vector<std::uint64_t, 2>(void);
template Eigen::Vector<std::uint64_t, 3> ReadBuffer::read_vector<std::uint64_t, 3>(void);
template Eigen::Vector<std::uint64_t, 4> ReadBuffer::read_vector<std::uint64_t, 4>(void);

template Eigen::Vector<std::int8_t, 2> ReadBuffer::read_vector<std::int8_t, 2>(void);
template Eigen::Vector<std::int8_t, 3> ReadBuffer::read_vector<std::int8_t, 3>(void);
template Eigen::Vector<std::int8_t, 4> ReadBuffer::read_vector<std::int8_t, 4>(void);

template Eigen::Vector<std::int16_t, 2> ReadBuffer::read_vector<std::int16_t, 2>(void);
template Eigen::Vector<std::int16_t, 3> ReadBuffer::read_vector<std::int16_t, 3>(void);
template Eigen::Vector<std::int16_t, 4> ReadBuffer::read_vector<std::int16_t, 4>(void);

template Eigen::Vector<std::int32_t, 2> ReadBuffer::read_vector<std::int32_t, 2>(void);
template Eigen::Vector<std::int32_t, 3> ReadBuffer::read_vector<std::int32_t, 3>(void);
template Eigen::Vector<std::int32_t, 4> ReadBuffer::read_vector<std::int32_t, 4>(void);

template Eigen::Vector<std::int64_t, 2> ReadBuffer::read_vector<std::int64_t, 2>(void);
template Eigen::Vector<std::int64_t, 3> ReadBuffer::read_vector<std::int64_t, 3>(void);
template Eigen::Vector<std::int64_t, 4> ReadBuffer::read_vector<std::int64_t, 4>(void);

template Eigen::Vector<float, 2> ReadBuffer::read_vector<float, 2>(void);
template Eigen::Vector<float, 3> ReadBuffer::read_vector<float, 3>(void);
template Eigen::Vector<float, 4> ReadBuffer::read_vector<float, 4>(void);

void ReadBuffer::read_bytes(std::span<std::byte> buffer)
{
    auto amount_to_read = std::min(buffer.size(), m_vector.size() - m_position);

    if(amount_to_read > 0) {
        std::copy(m_vector.cbegin() + m_position, m_vector.cbegin() + m_position + amount_to_read, buffer.begin());
    }

    m_position += buffer.size();
}

std::vector<std::byte> ReadBuffer::read_compressed(void)
{
    // Mode, the uncompressed size and the compressed size; the second one sizes the allocation
    if(!has_records(1, sizeof(std::uint8_t) + 2 * sizeof(std::uint32_t))) {
        return {};
    }

    auto mode = read<std::uint8_t>();
    auto raw_size = static_cast<std::size_t>(read<std::uint32_t>());
    auto payload_size = static_cast<std::size_t>(read<std::uint32_t>());

    if(!has_records(1, payload_size)) {
        return {};
    }

    if(COMPRESSION_NONE == mode) {
        if(payload_size != raw_size) {
            return {};
        }

        std::vector<std::byte> result;
        result.resize(raw_size);
        read_bytes(result);
        return result;
    }

    std::vector<std::byte> compressed;
    compressed.resize(payload_size);
    read_bytes(compressed);

    std::vector<std::byte> result;
    result.resize(raw_size);

    auto out_size = static_cast<mz_ulong>(raw_size);
    auto status = mz_uncompress(reinterpret_cast<unsigned char*>(result.data()), &out_size,
        reinterpret_cast<const unsigned char*>(compressed.data()), static_cast<mz_ulong>(payload_size));

    if(MZ_OK != status || out_size != raw_size) {
        return {};
    }

    return result;
}

WriteBuffer::WriteBuffer(const WriteBuffer& other)
{
    m_vector = other.m_vector;
}

std::size_t WriteBuffer::size(void) const
{
    return m_vector.size();
}

const std::byte* WriteBuffer::data(void) const
{
    return m_vector.data();
}

void WriteBuffer::reset(void)
{
    m_vector.clear();
}

void WriteBuffer::write_buffer(const WriteBuffer& other)
{
    m_vector.insert(m_vector.end(), other.m_vector.begin(), other.m_vector.end());
}

void WriteBuffer::write_bytes(std::span<const std::byte> data)
{
    m_vector.insert(m_vector.end(), data.begin(), data.end());
}

template<>
void WriteBuffer::write<std::byte>(const std::byte value)
{
    m_vector.push_back(value);
}

template<>
void WriteBuffer::write<std::uint8_t>(const std::uint8_t value)
{
    m_vector.push_back(static_cast<std::byte>(value));
}

template<>
void WriteBuffer::write<std::uint16_t>(const std::uint16_t value)
{
    auto value_little = value;

    if constexpr(std::endian::native == std::endian::big) {
        value_little = std::byteswap(value_little);
    }

    std::array<std::byte, sizeof(value_little)> bytes;
    std::memcpy(bytes.data(), &value_little, sizeof(value_little));
    m_vector.insert(m_vector.end(), bytes.cbegin(), bytes.cend());
}

template<>
void WriteBuffer::write<std::uint32_t>(const std::uint32_t value)
{
    auto value_little = value;

    if constexpr(std::endian::native == std::endian::big) {
        value_little = std::byteswap(value_little);
    }

    std::array<std::byte, sizeof(value_little)> bytes;
    std::memcpy(bytes.data(), &value_little, sizeof(value_little));
    m_vector.insert(m_vector.end(), bytes.cbegin(), bytes.cend());
}

template<>
void WriteBuffer::write<std::uint64_t>(const std::uint64_t value)
{
    auto value_little = value;

    if constexpr(std::endian::native == std::endian::big) {
        value_little = std::byteswap(value_little);
    }

    std::array<std::byte, sizeof(value_little)> bytes;
    std::memcpy(bytes.data(), &value_little, sizeof(value_little));
    m_vector.insert(m_vector.end(), bytes.cbegin(), bytes.cend());
}

template<>
void WriteBuffer::write(const float value)
{
    write(std::bit_cast<std::uint32_t>(value));
}

template<>
void WriteBuffer::write(const std::int8_t value)
{
    write(std::bit_cast<std::uint8_t>(value));
}

template<>
void WriteBuffer::write(const std::int16_t value)
{
    write(std::bit_cast<std::uint16_t>(value));
}

template<>
void WriteBuffer::write(const std::int32_t value)
{
    write(std::bit_cast<std::uint32_t>(value));
}

template<>
void WriteBuffer::write(const std::int64_t value)
{
    write(std::bit_cast<std::uint64_t>(value));
}

template<>
void WriteBuffer::write<std::string_view>(const std::string_view value)
{
    auto length = static_cast<std::uint16_t>(std::min<std::size_t>(value.size(), UINT16_MAX));

    write<std::uint16_t>(length);

    for(auto& character : value) {
        m_vector.push_back(static_cast<std::byte>(character));
    }
}

template<typename T, std::size_t N>
void WriteBuffer::write_vector(const Eigen::Vector<T, N>& value)
{
    for(std::size_t i = 0; i < N; ++i) {
        write<T>(value[i]);
    }
}

template void WriteBuffer::write_vector<std::uint8_t, 2>(const Eigen::Vector<std::uint8_t, 2>& value);
template void WriteBuffer::write_vector<std::uint8_t, 3>(const Eigen::Vector<std::uint8_t, 3>& value);
template void WriteBuffer::write_vector<std::uint8_t, 4>(const Eigen::Vector<std::uint8_t, 4>& value);

template void WriteBuffer::write_vector<std::uint16_t, 2>(const Eigen::Vector<std::uint16_t, 2>& value);
template void WriteBuffer::write_vector<std::uint16_t, 3>(const Eigen::Vector<std::uint16_t, 3>& value);
template void WriteBuffer::write_vector<std::uint16_t, 4>(const Eigen::Vector<std::uint16_t, 4>& value);

template void WriteBuffer::write_vector<std::uint32_t, 2>(const Eigen::Vector<std::uint32_t, 2>& value);
template void WriteBuffer::write_vector<std::uint32_t, 3>(const Eigen::Vector<std::uint32_t, 3>& value);
template void WriteBuffer::write_vector<std::uint32_t, 4>(const Eigen::Vector<std::uint32_t, 4>& value);

template void WriteBuffer::write_vector<std::uint64_t, 2>(const Eigen::Vector<std::uint64_t, 2>& value);
template void WriteBuffer::write_vector<std::uint64_t, 3>(const Eigen::Vector<std::uint64_t, 3>& value);
template void WriteBuffer::write_vector<std::uint64_t, 4>(const Eigen::Vector<std::uint64_t, 4>& value);

template void WriteBuffer::write_vector<std::int8_t, 2>(const Eigen::Vector<std::int8_t, 2>& value);
template void WriteBuffer::write_vector<std::int8_t, 3>(const Eigen::Vector<std::int8_t, 3>& value);
template void WriteBuffer::write_vector<std::int8_t, 4>(const Eigen::Vector<std::int8_t, 4>& value);

template void WriteBuffer::write_vector<std::int16_t, 2>(const Eigen::Vector<std::int16_t, 2>& value);
template void WriteBuffer::write_vector<std::int16_t, 3>(const Eigen::Vector<std::int16_t, 3>& value);
template void WriteBuffer::write_vector<std::int16_t, 4>(const Eigen::Vector<std::int16_t, 4>& value);

template void WriteBuffer::write_vector<std::int32_t, 2>(const Eigen::Vector<std::int32_t, 2>& value);
template void WriteBuffer::write_vector<std::int32_t, 3>(const Eigen::Vector<std::int32_t, 3>& value);
template void WriteBuffer::write_vector<std::int32_t, 4>(const Eigen::Vector<std::int32_t, 4>& value);

template void WriteBuffer::write_vector<std::int64_t, 2>(const Eigen::Vector<std::int64_t, 2>& value);
template void WriteBuffer::write_vector<std::int64_t, 3>(const Eigen::Vector<std::int64_t, 3>& value);
template void WriteBuffer::write_vector<std::int64_t, 4>(const Eigen::Vector<std::int64_t, 4>& value);

template void WriteBuffer::write_vector<float, 2>(const Eigen::Vector<float, 2>& value);
template void WriteBuffer::write_vector<float, 3>(const Eigen::Vector<float, 3>& value);
template void WriteBuffer::write_vector<float, 4>(const Eigen::Vector<float, 4>& value);

void WriteBuffer::write_compressed(std::span<const std::byte> data)
{
    auto raw_size = static_cast<mz_ulong>(data.size());
    auto compressed_size = mz_compressBound(raw_size);

    std::vector<std::byte> compressed;
    compressed.resize(compressed_size);

    auto status = mz_compress2(reinterpret_cast<unsigned char*>(compressed.data()), &compressed_size,
        reinterpret_cast<const unsigned char*>(data.data()), raw_size, MZ_BEST_SPEED);
    assert(MZ_OK == status);

    if(compressed_size < raw_size) {
        write<std::uint8_t>(COMPRESSION_ZLIB);
        write<std::uint32_t>(static_cast<std::uint32_t>(raw_size));
        write<std::uint32_t>(static_cast<std::uint32_t>(compressed_size));
        write_bytes(std::span(compressed.data(), static_cast<std::size_t>(compressed_size)));
    }
    else {
        write<std::uint8_t>(COMPRESSION_NONE);
        write<std::uint32_t>(static_cast<std::uint32_t>(raw_size));
        write<std::uint32_t>(static_cast<std::uint32_t>(raw_size));
        write_bytes(data);
    }
}

PHYSFS_File* WriteBuffer::to_file(std::string_view path, bool append) const
{
    std::string path_unfucked(path);
    PHYSFS_File* file = nullptr;

    if(append) {
        file = PHYSFS_openAppend(path_unfucked.c_str());
    }
    else {
        file = PHYSFS_openWrite(path_unfucked.c_str());
    }

    if(file) {
        PHYSFS_writeBytes(file, m_vector.data(), m_vector.size());
    }

    return file;
}

ENetPacket* WriteBuffer::to_packet(enet_uint32 flags) const
{
    return enet_packet_create(m_vector.data(), m_vector.size(), flags);
}

void WriteBuffer::to_stream(std::ostream& stream) const
{
    assert(stream.good());

    stream.write(reinterpret_cast<const char*>(m_vector.data()), m_vector.size());
}

std::vector<std::byte> WriteBuffer::take(void)
{
    return std::move(m_vector);
}
