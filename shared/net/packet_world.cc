#include "shared/pch.hh"

#include "shared/net/packet_world.hh"

#include "core/buffer.hh"

void packet::World_Request::encode(const World_Request& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 3>(packet.cpos.cast<std::int32_t>());
}

void packet::World_Request::decode(World_Request& packet, ReadBuffer& buffer)
{
    packet.cpos = buffer.read_vector<std::int32_t, 3>().cast<ChunkPos::value_type>();
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
    packet.cpos = buffer.read_vector<std::int32_t, 3>().cast<ChunkPos::value_type>();
    BlockStorage::decode_net(packet.blocks, buffer);
    LightStorage::decode_net(packet.light, buffer);
    packet.light_valid = buffer.read<std::uint8_t>() != 0;
}

void packet::World_Biomes::encode(const World_Biomes& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(static_cast<std::uint32_t>(packet.realm));
    buffer.write_vector<std::int32_t, 2>(packet.cpos.cast<std::int32_t>());
    BiomeSlice::encode(packet.biomes, buffer);
}

void packet::World_Biomes::decode(World_Biomes& packet, ReadBuffer& buffer)
{
    packet.realm = static_cast<biome_realm>(buffer.read<std::uint32_t>());
    packet.cpos = buffer.read_vector<std::int32_t, 2>().cast<ChunkPosXZ::value_type>();
    BiomeSlice::decode(packet.biomes, buffer);
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

        for(std::size_t i = 0; i < entry.indices.size(); i += 1) {
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

    for(std::uint16_t i = 0; i < count; i += 1) {
        // Chunk position plus the full flag.
        if(!buffer.has_records(1, 3 * sizeof(std::int32_t) + sizeof(std::uint8_t))) {
            return;
        }

        Entry entry;

        entry.cpos = buffer.read_vector<std::int32_t, 3>().cast<ChunkPos::value_type>();
        entry.full = buffer.read<std::uint8_t>() != 0;

        if(entry.full) {
            LightStorage::decode_net(entry.light, buffer);
            packet.entries.push_back(std::move(entry));
            continue;
        }

        if(!buffer.has_records(1, sizeof(std::uint16_t))) {
            return;
        }

        auto cells = buffer.read<std::uint16_t>();

        if(!buffer.has_records(cells, sizeof(std::uint16_t) + sizeof(std::uint8_t))) {
            return;
        }

        entry.indices.reserve(cells);
        entry.values.reserve(cells);

        for(std::uint16_t j = 0; j < cells; j += 1) {
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
    packet.bpos = buffer.read_vector<std::int64_t, 3>().cast<BlockPos::value_type>();
    packet.block = buffer.read<std::uint32_t>();
}

void packet::World_Timings::encode(const World_Timings& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint64_t>(packet.current_tick);
}

void packet::World_Timings::decode(World_Timings& packet, ReadBuffer& buffer)
{
    packet.current_tick = buffer.read<std::uint64_t>();
}
