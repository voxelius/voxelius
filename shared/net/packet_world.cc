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
    buffer.write_vector<std::int32_t, 3>(packet.cpos.cast<std::int32_t>());
    LightStorage::encode_net(packet.light, buffer);
}

void packet::World_Light::decode(World_Light& packet, ReadBuffer& buffer)
{
    packet.cpos = buffer.read_vector<std::int32_t, 3>().cast<ChunkPos::value_type>();
    LightStorage::decode_net(packet.light, buffer);
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
