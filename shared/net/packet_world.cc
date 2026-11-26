#include "shared/pch.hh"

#include "shared/net/packet_world.hh"

#include "core/buffer.hh"

#include "shared/world/world_definition.hh"

constexpr static std::size_t ENTRY_HEADER_SIZE = 3 * sizeof(std::int32_t) + sizeof(std::uint8_t);
constexpr static std::size_t CELL_SIZE = sizeof(std::uint16_t) + sizeof(std::uint8_t);
constexpr static std::size_t COLUMN_BYTES = 2 * sizeof(std::int32_t);

void packet::World_Request::encode(const World_Request& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 3>(packet.view.min());
    buffer.write_vector<std::int32_t, 3>(packet.view.max());
    buffer.write_vector<std::int32_t, 2>(packet.interest.min());
    buffer.write_vector<std::int32_t, 2>(packet.interest.max());
    buffer.write<std::uint16_t>(packet.chunk_budget);
}

void packet::World_Request::decode(World_Request& packet, ReadBuffer& buffer)
{
    auto view_min = buffer.read_vector<std::int32_t, 3>();
    auto view_max = buffer.read_vector<std::int32_t, 3>();
    auto interest_min = buffer.read_vector<std::int32_t, 2>();
    auto interest_max = buffer.read_vector<std::int32_t, 2>();

    packet.view = ChunkAlignedBox(view_min, view_max);
    packet.interest = ChunkAlignedBoxXZ(interest_min, interest_max);
    packet.chunk_budget = buffer.read<std::uint16_t>();
}

void packet::World_Blocks::encode(const World_Blocks& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 3>(packet.cpos.cast<std::int32_t>());
    BlockStorage::encode_net(packet.blocks, buffer);
    LightStorage::encode_net(packet.light, buffer);
    buffer.write<std::uint8_t>(packet.light_valid ? 1 : 0);
}

void packet::World_Blocks::decode(World_Blocks& packet, ReadBuffer& buffer)
{
    auto cpos = buffer.read_vector<std::int32_t, 3>();
    packet.cpos = cpos.cast<ChunkPos::value_type>();
    BlockStorage::decode_net(packet.blocks, buffer);
    LightStorage::decode_net(packet.light, buffer);
    packet.light_valid = buffer.read<std::uint8_t>();
}

void packet::World_Biomes::encode(const World_Biomes& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 2>(packet.cpos.cast<std::int32_t>());
    BiomeSlice::encode(packet.biomes, buffer);
}

void packet::World_Biomes::decode(World_Biomes& packet, ReadBuffer& buffer)
{
    auto cpos = buffer.read_vector<std::int32_t, 2>();
    packet.cpos = cpos.cast<ChunkPosXZ::value_type>();
    BiomeSlice::decode(packet.biomes, buffer);
}

void packet::World_Columns::encode(const World_Columns& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 2>(packet.cpos.cast<std::int32_t>());

    WriteBuffer raw;

    for(const auto& column : packet.columns) {
        Column::encode(column, raw);
    }

    buffer.write_compressed(std::span(raw.data(), raw.size()));
}

void packet::World_Columns::decode(World_Columns& packet, ReadBuffer& buffer)
{
    auto cpos = buffer.read_vector<std::int32_t, 2>();
    packet.cpos = cpos.cast<ChunkPosXZ::value_type>();
    packet.columns.fill({});

    auto raw = buffer.read_compressed();
    auto raw_span = std::span<const std::byte>(raw);
    auto source = ReadBuffer(raw_span);

    for(auto& column : packet.columns) {
        if(!source.has_records(1, COLUMN_BYTES)) {
            return;
        }

        Column::decode(column, source);
    }
}

void packet::World_Light::encode(const World_Light& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(packet.entries.size()));

    for(const auto& entry : packet.entries) {
        buffer.write_vector<std::int32_t, 3>(entry.cpos.cast<std::int32_t>());
        buffer.write<std::uint8_t>(entry.full ? 1 : 0);

        if(entry.full) {
            LightStorage::encode_net(entry.light, buffer);
            continue;
        }

        buffer.write<std::uint16_t>(static_cast<std::uint16_t>(entry.indices.size()));

        for(std::size_t i = 0; i < entry.indices.size(); ++i) {
            buffer.write<std::uint16_t>(entry.indices[i]);
            buffer.write<std::uint8_t>(entry.values[i]);
        }
    }
}

void packet::World_Light::decode(World_Light& packet, ReadBuffer& buffer)
{
    packet.entries.clear();

    if(!buffer.has_records(1, sizeof(std::uint16_t))) {
        return;
    }

    auto count = buffer.read<std::uint16_t>();

    for(std::uint16_t i = 0; i < count; ++i) {
        if(!buffer.has_records(1, ENTRY_HEADER_SIZE)) {
            return;
        }

        Entry entry;

        auto cpos = buffer.read_vector<std::int32_t, 3>();
        entry.cpos = cpos.cast<ChunkPos::value_type>();
        entry.full = buffer.read<std::uint8_t>();

        if(entry.full) {
            LightStorage::decode_net(entry.light, buffer);
            packet.entries.push_back(std::move(entry));
            continue;
        }

        if(!buffer.has_records(1, sizeof(std::uint16_t))) {
            return;
        }

        auto cells = buffer.read<std::uint16_t>();

        if(!buffer.has_records(cells, CELL_SIZE)) {
            return;
        }

        entry.indices.reserve(cells);
        entry.values.reserve(cells);

        for(std::uint16_t j = 0; j < cells; ++j) {
            entry.indices.push_back(buffer.read<std::uint16_t>());
            entry.values.push_back(buffer.read<std::uint8_t>());
        }

        packet.entries.push_back(std::move(entry));
    }
}

void packet::World_SetBlock::encode(const World_SetBlock& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int64_t, 3>(packet.bpos.cast<std::int64_t>());
    buffer.write<std::uint32_t>(packet.block);
}

void packet::World_SetBlock::decode(World_SetBlock& packet, ReadBuffer& buffer)
{
    auto bpos = buffer.read_vector<std::int64_t, 3>();
    packet.bpos = bpos.cast<BlockPos::value_type>();
    packet.block = buffer.read<std::uint32_t>();
}

void packet::World_Column::encode(const World_Column& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int64_t, 2>(packet.bpos.cast<std::int64_t>());
    Column::encode(packet.column, buffer);
}

void packet::World_Column::decode(World_Column& packet, ReadBuffer& buffer)
{
    auto bpos = buffer.read_vector<std::int64_t, 2>();
    packet.bpos = bpos.cast<BlockPosXZ::value_type>();
    Column::decode(packet.column, buffer);
}

void packet::World_Timings::encode(const World_Timings& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint64_t>(packet.current_tick);
    buffer.write<std::uint64_t>(packet.daylight_tick);
    buffer.write<std::uint64_t>(packet.day);
    buffer.write<std::uint64_t>(packet.weather_tick);
    buffer.write<std::uint64_t>(packet.season_tick);
}

void packet::World_Timings::decode(World_Timings& packet, ReadBuffer& buffer)
{
    packet.current_tick = buffer.read<std::uint64_t>();
    packet.daylight_tick = buffer.read<std::uint64_t>();
    packet.day = buffer.read<std::uint64_t>();
    packet.weather_tick = buffer.read<std::uint64_t>();
    packet.season_tick = buffer.read<std::uint64_t>();
}

void packet::World_Info::encode(const World_Info& packet, WriteBuffer& buffer)
{
    buffer.write<std::string_view>(packet.name);
    buffer.write<std::uint64_t>(packet.weather_seed);

    buffer.write<std::int64_t>(packet.min_y);
    buffer.write<std::int64_t>(packet.max_y);
    buffer.write_vector<std::int64_t, 3>(packet.spawn);

    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(packet.weather.size()));

    for(auto type : packet.weather) {
        buffer.write<std::uint32_t>(type);
    }

    WorldVisuals::encode(packet.visuals, buffer);
}

void packet::World_Info::decode(World_Info& packet, ReadBuffer& buffer)
{
    packet.name = buffer.read<std::string>();
    packet.weather_seed = buffer.read<std::uint64_t>();

    packet.min_y = buffer.read<std::int64_t>();
    packet.max_y = buffer.read<std::int64_t>();
    packet.spawn = buffer.read_vector<std::int64_t, 3>();

    packet.weather.clear();

    if(buffer.has_records(1, sizeof(std::uint16_t))) {
        auto count = buffer.read<std::uint16_t>();

        if(buffer.has_records(count, sizeof(std::uint32_t))) {
            packet.weather.reserve(count);

            for(std::uint16_t i = 0; i < count; ++i) {
                packet.weather.push_back(buffer.read<std::uint32_t>());
            }
        }
    }

    WorldVisuals::decode(packet.visuals, buffer);
}

void packet::World_Info::fill(const WorldDefinition& definition)
{
    name = definition.name;
    min_y = definition.min_y;
    max_y = definition.max_y;
    spawn = definition.spawn;
    weather = definition.weather;
    visuals = definition.visuals;
}

void packet::World_Info::to_definition(WorldDefinition& definition) const
{
    definition.name = name;
    definition.min_y = min_y;
    definition.max_y = max_y;
    definition.spawn = spawn;
    definition.weather = weather;
    definition.visuals = visuals;
}
