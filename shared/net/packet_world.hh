#ifndef BD2D77E8_727E_4B4C_83D6_92E1569F5B4D
#define BD2D77E8_727E_4B4C_83D6_92E1569F5B4D

#include "shared/coord.hh"
#include "shared/net/packet.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/light_storage.hh"
#include "shared/world/weather.hh"
#include "shared/world/world_visuals.hh"

struct WorldDefinition;

struct packet::World_Request final : public packet::Base<packet::WORLD_REQUEST_CHUNK> {
    static void encode(const World_Request& packet, WriteBuffer& buffer);
    static void decode(World_Request& packet, ReadBuffer& buffer);

    ChunkAlignedBox view;
    std::uint16_t chunk_budget { 0 };
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

    ChunkPosXZ cpos;
    BiomeSlice biomes;
};

struct packet::World_Light final : public packet::Base<packet::WORLD_CHUNK_LIGHT> {
    struct Entry final {
        ChunkPos cpos;

        // A delta only names the cells that changed; a full entry carries the
        // whole array, which is what a chunk nobody has seen yet needs.
        std::vector<std::uint16_t> indices;
        std::vector<std::uint8_t> values;
        LightStorage light;
        bool full { true };
    };

    static void encode(const World_Light& packet, WriteBuffer& buffer);
    static void decode(World_Light& packet, ReadBuffer& buffer);

    // One packet carries every chunk whose light changed in a tick. Sending a
    // packet per chunk cost a thousand small packets a second while the world
    // was streaming in.
    std::vector<Entry> entries;
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
    std::uint64_t daylight_tick;
    std::uint64_t day;
};

struct packet::World_Info final : public packet::Base<packet::WORLD_INFO> {
    static void encode(const World_Info& packet, WriteBuffer& buffer);
    static void decode(World_Info& packet, ReadBuffer& buffer);

    void fill(const WorldDefinition& definition);
    void to_definition(WorldDefinition& definition) const;

    std::string id;
    std::uint64_t seed;
    std::uint32_t star_seed;

    std::uint32_t types_seed;
    float types_scale;
    std::uint32_t detail_seed;
    float detail_scale;

    BlockPos::value_type min_y { -64 };
    BlockPos::value_type max_y { 255 };
    BlockPos spawn { BlockPos::Zero() };
    std::vector<weather_type_id> weather;
    WorldVisuals visuals;
};

struct packet::World_Weather final : public packet::Base<packet::WORLD_WEATHER> {
    static void encode(const World_Weather& packet, WriteBuffer& buffer);
    static void decode(World_Weather& packet, ReadBuffer& buffer);

    Eigen::Vector2f types_drift;
    Eigen::Vector2f detail_drift;
    Eigen::Vector2f wind;
};

#endif /* BD2D77E8_727E_4B4C_83D6_92E1569F5B4D */
