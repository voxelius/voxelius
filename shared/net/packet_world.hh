#ifndef BD2D77E8_727E_4B4C_83D6_92E1569F5B4D
#define BD2D77E8_727E_4B4C_83D6_92E1569F5B4D

#include "shared/net/packet.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/light_storage.hh"

struct packet::World_Request final : public packet::Base<packet::WORLD_REQUEST_CHUNK> {
    static void encode(const World_Request& packet, WriteBuffer& buffer);
    static void decode(World_Request& packet, ReadBuffer& buffer);

    ChunkPos cpos;
};

struct packet::World_Blocks final : public packet::Base<packet::WORLD_CHUNK_BLOCKS> {
    static void encode(const World_Blocks& packet, WriteBuffer& buffer);
    static void decode(World_Blocks& packet, ReadBuffer& buffer);

    ChunkPos cpos;
    BlockStorage blocks;
    LightStorage light;
    bool light_valid { false };
};

struct packet::World_Biomes final : public packet::Base<packet::WORLD_CHUNK_BIOMES> {
    static void encode(const World_Biomes& packet, WriteBuffer& buffer);
    static void decode(World_Biomes& packet, ReadBuffer& buffer);

    biome_realm realm;
    ChunkPosXZ cpos;
    BiomeSlice biomes;
};

struct packet::World_Light final : public packet::Base<packet::WORLD_CHUNK_LIGHT> {
    static void encode(const World_Light& packet, WriteBuffer& buffer);
    static void decode(World_Light& packet, ReadBuffer& buffer);

    ChunkPos cpos;
    LightStorage light;
};

struct packet::World_SetBlock final : public packet::Base<packet::WORLD_SET_BLOCK> {
    static void encode(const World_SetBlock& packet, WriteBuffer& buffer);
    static void decode(World_SetBlock& packet, ReadBuffer& buffer);

    BlockPos bpos;
    block_id_type block;
};

struct packet::World_Timings final : public packet::Base<packet::WORLD_TIMINGS> {
    static void encode(const World_Timings& packet, WriteBuffer& buffer);
    static void decode(World_Timings& packet, ReadBuffer& buffer);

    std::uint64_t current_tick;
};

#endif /* BD2D77E8_727E_4B4C_83D6_92E1569F5B4D */
