#include "shared/pch.hh"

#include "shared/world/block_storage.hh"

#include "core/buffer.hh"

#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"

constexpr static std::uint32_t TAG_UNIFORM = 0x85787370;   // UNIF
constexpr static std::uint32_t TAG_PALETTE8 = 0x80657666;  // PALB
constexpr static std::uint32_t TAG_PALETTE16 = 0x80657687; // PALW

constexpr static std::uint8_t COMPRESSION_NONE = 0;
constexpr static std::uint8_t COMPRESSION_ZLIB = 1;

static void encode_block_id(block_id_type id, WriteBuffer& buffer)
{
    if(auto family = block_registry::find_family_of(id)) {
        buffer.write<std::string_view>(family->name.full_string());

        auto it = family->id_states.find(id);

        if(it == family->id_states.cend()) {
            buffer.write<std::uint16_t>(0);
            return;
        }

        buffer.write<std::uint16_t>(static_cast<std::uint16_t>(it->second.size()));

        for(const auto& it : it->second) {
            buffer.write<std::uint64_t>(it.first);
            buffer.write<std::uint64_t>(it.second);
        }

        return;
    }

    if(auto name = block_registry::name_of(id)) {
        buffer.write<std::string_view>(name->full_string());
        buffer.write<std::uint16_t>(0);
        return;
    }

    buffer.write<std::string_view>({});
    buffer.write<std::uint16_t>(0);
}

static block_id_type decode_block_id(ReadBuffer& buffer)
{
    auto name = buffer.read<std::string>();
    auto num_states = buffer.read<std::uint16_t>();

    vx::hash_map<blockstate_key_type, blockstate_val_type> states;

    for(std::uint16_t i = 0; i < num_states; ++i) {
        blockstate_key_type name_hash = buffer.read<std::uint64_t>();
        blockstate_val_type value_hash = buffer.read<std::uint64_t>();
        states.insert_or_assign(std::move(name_hash), std::move(value_hash));
    }

    if(name.empty()) {
        return BLOCK_ID_NULL;
    }

    auto identifier = Identifier::from_string(name);

    if(num_states == 0) {
        if(auto block_id = block_registry::find(identifier)) {
            return block_id;
        }
    }

    if(auto family = block_registry::find_family(identifier)) {
        return block_registry::resolve_variant(family->stem_id, states);
    }

    return BLOCK_ID_NULL;
}

void BlockStorage::encode_net(const BlockStorage& storage, WriteBuffer& buffer)
{
    WriteBuffer raw;

    if(auto uniform = std::get_if<Uniform>(&storage.m_variant)) {
        raw.write<std::uint32_t>(TAG_UNIFORM);
        encode_net(uniform, raw);
    }
    else if(auto p8 = std::get_if<Palette8>(&storage.m_variant)) {
        raw.write<std::uint32_t>(TAG_PALETTE8);
        encode_net(p8, raw);
    }
    else if(auto p16 = std::get_if<Palette16>(&storage.m_variant)) {
        raw.write<std::uint32_t>(TAG_PALETTE16);
        encode_net(p16, raw);
    }

    auto raw_size = static_cast<mz_ulong>(raw.size());
    auto raw_bound = mz_compressBound(raw_size);

    auto compressed_size = raw_bound;
    std::vector<std::byte> compressed;
    compressed.resize(raw_bound);

    auto raw_data = reinterpret_cast<const unsigned char*>(raw.data());
    auto compressed_data = reinterpret_cast<unsigned char*>(compressed.data());
    auto status = mz_compress2(compressed_data, &compressed_size, raw_data, raw_size, MZ_DEFAULT_LEVEL);
    assert(status == MZ_OK);

    if(compressed_size < raw_size) {
        buffer.write<std::uint8_t>(COMPRESSION_ZLIB);
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(raw_size));
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(compressed_size));
        buffer.write_bytes(std::span<const std::byte>(compressed.data(), compressed_size));
    }
    else {
        buffer.write<std::uint8_t>(COMPRESSION_NONE);
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(raw_size));
        buffer.write_bytes(std::span(raw.data(), raw.size()));
    }
}

void BlockStorage::decode_net(BlockStorage& storage, ReadBuffer& buffer)
{
    auto mode = buffer.read<std::uint8_t>();
    auto raw_size = buffer.read<std::uint32_t>();

    std::vector<std::byte> raw_bytes;
    raw_bytes.resize(raw_size);

    if(mode == COMPRESSION_ZLIB) {
        auto compressed_size = static_cast<mz_ulong>(buffer.read<std::uint32_t>());

        mz_ulong raw_size_mz = raw_size;
        std::vector<std::byte> compressed;
        compressed.resize(compressed_size);
        buffer.read_bytes(compressed);

        auto raw_data = reinterpret_cast<unsigned char*>(raw_bytes.data());
        auto compressed_data = reinterpret_cast<unsigned char*>(compressed.data());
        auto status = mz_uncompress(raw_data, &raw_size_mz, compressed_data, compressed_size);
        assert(status == MZ_OK);
    }
    else {
        buffer.read_bytes(raw_bytes);
    }

    ReadBuffer raw;
    raw.reset(raw_bytes);

    Palette8 p8;
    Palette16 p16;
    Uniform uniform;

    switch(raw.read<std::uint32_t>()) {
        case TAG_UNIFORM:
            decode_net(uniform, raw);
            storage.m_variant = std::move(uniform);
            return;

        case TAG_PALETTE8:
            decode_net(p8, raw);
            storage.m_variant = std::move(p8);
            return;

        case TAG_PALETTE16:
            decode_net(p16, raw);
            storage.m_variant = std::move(p16);
            return;
    }
}

void BlockStorage::encode_dat(const BlockStorage& storage, WriteBuffer& buffer)
{
    thread_local std::array<block_id_type, constant::CHUNK_VOLUME> flat;
    thread_local std::vector<block_id_type> local_palette;
    thread_local vx::hash_map<block_id_type, std::uint16_t> local_slots;
    thread_local std::vector<std::uint16_t> local_indices;

    storage.flatten(flat);
    local_palette.clear();
    local_slots.clear();
    local_indices.resize(constant::CHUNK_VOLUME);

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto it = local_slots.find(flat[i]);

        if(it == local_slots.cend()) {
            auto slot = static_cast<std::uint16_t>(local_palette.size());
            local_palette.push_back(flat[i]);

            block_id_type key = flat[i];
            local_slots.insert_or_assign(std::move(key), std::move(slot));
            local_indices[i] = slot;
        }
        else {
            local_indices[i] = it->second;
        }
    }

    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(local_palette.size()));

    for(auto id : local_palette) {
        encode_block_id(id, buffer);
    }

    for(auto index : local_indices) {
        buffer.write<std::uint16_t>(index);
    }
}

void BlockStorage::decode_dat(BlockStorage& storage, ReadBuffer& buffer)
{
    thread_local std::vector<block_id_type> local_palette;

    auto palette_size = buffer.read<std::uint16_t>();

    local_palette.clear();

    if(palette_size == 0) {
        local_palette.push_back(BLOCK_ID_NULL);
    }
    else {
        for(std::size_t i = 0; i < palette_size; ++i) {
            local_palette.push_back(decode_block_id(buffer));
        }
    }

    storage.fill(BLOCK_ID_NULL);

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto index = buffer.read<std::uint16_t>();

        if(index < local_palette.size()) {
            storage.set(i, local_palette[index]);
        }
    }

    storage.optimize();
}

void BlockStorage::encode_net(const Uniform* uniform, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(uniform->filler);
}

void BlockStorage::encode_net(const Palette8* p8, WriteBuffer& buffer)
{
    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(p8->palette.size()));

    for(const auto& [id, refcount] : p8->palette) {
        buffer.write<std::uint32_t>(id);
        buffer.write<std::uint16_t>(refcount);
    }

    for(const auto index : p8->indices) {
        buffer.write<std::uint8_t>(index);
    }
}

void BlockStorage::encode_net(const Palette16* p16, WriteBuffer& buffer)
{
    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(p16->palette.size()));

    for(const auto& [id, refcount] : p16->palette) {
        buffer.write<std::uint32_t>(id);
        buffer.write<std::uint16_t>(refcount);
    }

    for(const auto index : p16->indices) {
        buffer.write<std::uint16_t>(index);
    }
}

void BlockStorage::decode_net(Uniform& uniform, ReadBuffer& buffer)
{
    uniform.filler = buffer.read<std::uint32_t>();
}

void BlockStorage::decode_net(Palette8& p8, ReadBuffer& buffer)
{
    p8.palette.resize(buffer.read<std::uint16_t>());

    for(auto& [id, refcount] : p8.palette) {
        id = buffer.read<std::uint32_t>();
        refcount = buffer.read<std::uint16_t>();
    }

    if(p8.palette.empty()) {
        p8.palette.emplace_back(BLOCK_ID_NULL, static_cast<std::uint16_t>(constant::CHUNK_VOLUME));
    }

    const auto max_slot = static_cast<std::uint8_t>(p8.palette.size() - 1);

    for(auto& index : p8.indices) {
        index = std::min(buffer.read<std::uint8_t>(), max_slot);
    }
}

void BlockStorage::decode_net(Palette16& p16, ReadBuffer& buffer)
{
    p16.palette.resize(buffer.read<std::uint16_t>());

    for(auto& [id, refcount] : p16.palette) {
        id = buffer.read<std::uint32_t>();
        refcount = buffer.read<std::uint16_t>();
    }

    if(p16.palette.empty()) {
        p16.palette.emplace_back(BLOCK_ID_NULL, static_cast<std::uint16_t>(constant::CHUNK_VOLUME));
    }

    const auto max_slot = static_cast<std::uint16_t>(p16.palette.size() - 1);

    for(auto& index : p16.indices) {
        index = std::min(buffer.read<std::uint16_t>(), max_slot);
    }
}

std::optional<std::size_t> BlockStorage::find_slot(const palette_type& palette, block_id_type id)
{
    for(std::size_t i = 0; i < palette.size(); ++i) {
        if(palette[i].first == id) {
            return i;
        }
    }

    return std::nullopt;
}

std::size_t BlockStorage::add_slot(palette_type& palette, block_id_type id)
{
    auto slot = palette.size();
    palette.emplace_back(id, UINT16_C(0));
    return slot;
}

void BlockStorage::promote(Palette8& dest, const Uniform& src)
{
    dest.indices.fill(0);
    dest.palette.clear();
    dest.palette.emplace_back(src.filler, static_cast<std::uint16_t>(constant::CHUNK_VOLUME));
}

void BlockStorage::promote(Palette16& dest, const Palette8& src)
{
    dest.palette = src.palette;

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        dest.indices[i] = src.indices[i];
    }
}

void BlockStorage::optimize(Palette8& p8, variant_type& dest)
{
    palette_type compact;
    std::vector<std::uint8_t> remap;
    remap.resize(p8.palette.size(), 0);

    for(std::size_t i = 0; i < p8.palette.size(); ++i) {
        if(p8.palette[i].second > 0) {
            remap[i] = static_cast<std::uint8_t>(compact.size());
            compact.push_back(p8.palette[i]);
        }
    }

    if(compact.empty()) {
        dest = Uniform { BLOCK_ID_NULL };
        return;
    }

    if(1 == compact.size()) {
        dest = Uniform { compact[0].first };
        return;
    }

    for(auto& index : p8.indices) {
        index = remap[index];
    }

    p8.palette = std::move(compact);
}

void BlockStorage::optimize(Palette16& p16, variant_type& dest)
{
    palette_type compact;
    std::vector<std::uint16_t> remap;
    remap.resize(p16.palette.size(), 0);

    for(std::size_t i = 0; i < p16.palette.size(); ++i) {
        if(p16.palette[i].second > 0) {
            remap[i] = static_cast<std::uint16_t>(compact.size());
            compact.push_back(p16.palette[i]);
        }
    }

    if(compact.empty()) {
        dest = Uniform { BLOCK_ID_NULL };
        return;
    }

    if(1 == compact.size()) {
        dest = Uniform { compact[0].first };
        return;
    }

    for(auto& index : p16.indices) {
        index = remap[index];
    }

    p16.palette = std::move(compact);

    if(p16.palette.size() <= 256) {
        Palette8 p8;
        p8.palette = p16.palette;

        for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
            p8.indices[i] = static_cast<std::uint8_t>(p16.indices[i]);
        }

        dest = std::move(p8);
    }
}

block_id_type BlockStorage::get(std::size_t index) const
{
    if(index >= constant::CHUNK_VOLUME) {
        return BLOCK_ID_NULL;
    }

    if(const auto uniform = std::get_if<Uniform>(&m_variant)) {
        return uniform->filler;
    }

    if(const auto p8 = std::get_if<Palette8>(&m_variant)) {
        return p8->palette[p8->indices[index]].first;
    }

    if(const auto p16 = std::get_if<Palette16>(&m_variant)) {
        return p16->palette[p16->indices[index]].first;
    }

    return BLOCK_ID_NULL;
}

block_id_type BlockStorage::get(const LocalPos& pos) const
{
    return get(utils::to_index(pos));
}

void BlockStorage::flatten(std::span<block_id_type> out) const
{
    assert(out.size() >= constant::CHUNK_VOLUME);

    if(const auto uniform = std::get_if<Uniform>(&m_variant)) {
        std::fill_n(out.data(), constant::CHUNK_VOLUME, uniform->filler);
        return;
    }

    if(const auto p8 = std::get_if<Palette8>(&m_variant)) {
        for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
            out[i] = p8->palette[p8->indices[i]].first;
        }

        return;
    }

    if(const auto p16 = std::get_if<Palette16>(&m_variant)) {
        for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
            out[i] = p16->palette[p16->indices[i]].first;
        }

        return;
    }

    std::fill_n(out.data(), constant::CHUNK_VOLUME, BLOCK_ID_NULL);
}

void BlockStorage::set(std::size_t index, block_id_type id)
{
    if(index >= constant::CHUNK_VOLUME) {
        return;
    }

    if(const auto uniform = std::get_if<Uniform>(&m_variant)) {
        if(uniform->filler == id) {
            return;
        }

        Palette8 p8;
        promote(p8, *uniform);
        m_variant = std::move(p8);

        set(index, id);

        return;
    }

    if(auto p8 = std::get_if<Palette8>(&m_variant)) {
        auto slot = find_slot(p8->palette, id);

        if(!slot.has_value() && p8->palette.size() >= 256) {
            Palette16 p16;
            promote(p16, *p8);
            m_variant = std::move(p16);

            set(index, id);

            return;
        }

        std::size_t new_slot = 0;

        if(slot.has_value()) {
            new_slot = slot.value();
        }
        else {
            new_slot = add_slot(p8->palette, id);
        }

        p8->palette[p8->indices[index]].second -= 1;
        p8->indices[index] = static_cast<std::uint8_t>(new_slot);
        p8->palette[new_slot].second += 1;

        return;
    }

    if(auto p16 = std::get_if<Palette16>(&m_variant)) {
        auto slot = find_slot(p16->palette, id);
        std::size_t new_slot = 0;

        if(slot.has_value()) {
            new_slot = slot.value();
        }
        else {
            new_slot = add_slot(p16->palette, id);
        }

        p16->palette[p16->indices[index]].second -= 1;
        p16->indices[index] = static_cast<std::uint16_t>(new_slot);
        p16->palette[new_slot].second += 1;

        return;
    }
}

void BlockStorage::set(const LocalPos& pos, block_id_type id)
{
    set(utils::to_index(pos), id);
}

void BlockStorage::fill(block_id_type id)
{
    Uniform uniform {};
    uniform.filler = id;
    m_variant = std::move(uniform);
}

void BlockStorage::optimize(void)
{
    if(auto p8 = std::get_if<Palette8>(&m_variant)) {
        optimize(*p8, m_variant);
        return;
    }

    if(auto p16 = std::get_if<Palette16>(&m_variant)) {
        optimize(*p16, m_variant);
        return;
    }
}

std::size_t BlockStorage::size(void) const
{
    if(std::holds_alternative<Uniform>(m_variant)) {
        return sizeof(Uniform);
    }

    if(const auto p8 = std::get_if<Palette8>(&m_variant)) {
        std::size_t result = sizeof(Palette8);
        result += p8->palette.size() * sizeof(palette_type::value_type);
        return result;
    }

    if(const auto p16 = std::get_if<Palette16>(&m_variant)) {
        std::size_t result = sizeof(Palette16);
        result += p16->palette.size() * sizeof(palette_type::value_type);
        return result;
    }

    return 0;
}
