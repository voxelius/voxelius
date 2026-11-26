#ifndef B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71
#define B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71

#include <random>

#include "shared/world/biome_slice.hh"
#include "shared/world/world.hh"

#include "server/constant.hh"
#include "server/world/columns.hh"
#include "server/world/placement_rule.hh"
#include "server/world/world_definition.hh"

using CachedEntropy = std::array<std::uint64_t, constant::CHUNK_AREA>;

class ServerWorld final : public World {
public:
    struct ScatterEntry final {
        std::once_flag init_flag;
        std::vector<FeatureInstance> data;
    };

    explicit ServerWorld(std::shared_ptr<const ServerWorldDefinition> definition);

    constexpr const GeneratorDefinition* generator(void) const;

    constexpr void* generator_state(void) const;

    const CachedEntropy& sample_entropy(const ChunkPosXZ& pos);
    std::uint64_t entropy_at(const BlockPosXZ& pos);

    biome_id_type biome_at(const BlockPosXZ& pos);
    const BiomeSlice& sample_biomes(const ChunkPosXZ& pos);

    std::shared_ptr<ScatterEntry> scatter_entry(const ChunkPosXZ& pos);

    void commit_placements(BlockStorage& storage, const ChunkPos& cpos);
    void commit_placements(BlockStorage& storage, const ChunkPos& cpos, std::span<const FeatureInstance> instances);

    void init_generation(void);

    void schedule(const ChunkPos& cpos, const LocalPos& lpos, std::uint64_t deadline, block_tick_source source);
    void schedule(const BlockPos& pos, std::uint64_t deadline, block_tick_source source);

    void fixed_update(void);

    WorldColumns& columns(void);

    std::mt19937_64 weather_random;
    float weather_strike_budget { 0.0f };

private:
    struct EntropyEntry final {
        std::once_flag init_flag;
        CachedEntropy data;
    };

    struct BiomeEntry final {
        std::once_flag init_flag;
        BiomeSlice data;
    };

    std::shared_ptr<EntropyEntry> find_entropy_entry(const ChunkPosXZ& pos) const;
    std::shared_ptr<EntropyEntry> get_entropy_entry(const ChunkPosXZ& pos);

    std::shared_ptr<BiomeEntry> find_biome_entry(const ChunkPosXZ& pos) const;
    std::shared_ptr<BiomeEntry> get_biome_entry(const ChunkPosXZ& pos);

    std::shared_ptr<ScatterEntry> find_scatter(const ChunkPosXZ& pos) const;

    const std::vector<FeatureInstance>& collect_placements(const ChunkPosXZ& pos);

    std::shared_ptr<const ServerWorldDefinition> m_server_definition;

    std::uint64_t m_entropy_seed { 0 };
    mutable std::shared_mutex m_entropy_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<EntropyEntry>> m_entropy_cache;

    mutable std::shared_mutex m_biome_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<BiomeEntry>> m_biome_cache;

    mutable std::shared_mutex m_scatter_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<ScatterEntry>> m_scatter_cache;

    std::vector<std::unique_ptr<PlacementRule>> m_placement_rules;
    std::size_t m_placement_extent { 0 };

    WorldColumns m_columns;
};

constexpr const GeneratorDefinition* ServerWorld::generator(void) const
{
    return m_server_definition->generator;
}

constexpr void* ServerWorld::generator_state(void) const
{
    return m_server_definition->generator_state.get();
}

inline WorldColumns& ServerWorld::columns(void)
{
    return m_columns;
}

#endif /* B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71 */
