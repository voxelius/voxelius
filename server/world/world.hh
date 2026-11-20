#ifndef B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71
#define B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71

#include "server/constant.hh"
#include "server/world/noise_cache_2D.hh"
#include "server/world/placement_rule.hh"
#include "shared/world/biome.hh"
#include "shared/world/world.hh"

constexpr static std::size_t CLIMATE_DIMS = 5;

using ClimatePos = std::array<std::uint8_t, CLIMATE_DIMS>;

struct ClimateSeed final {
    ClimatePos target;
    biome_id_type id;
    unsigned priority;
    float offset;
};

struct ClimateSample final {
    float temperature;
    float humidity;
    float continentalness;
    float erosion;
    float weirdness;
};

using ClimateSampleArray = std::array<ClimateSample, constant::CHUNK_AREA>;
using CachedEntropy = std::array<std::uint64_t, constant::CHUNK_AREA>;

struct GeneratorDefinition;

class ServerWorld final : public World {
public:
    struct ScatterEntry final {
        std::once_flag init_flag;
        std::vector<FeatureInstance> data;
    };

    explicit ServerWorld(std::shared_ptr<const WorldDefinition> definition);

    constexpr const GeneratorDefinition* generator(void) const;
    void set_generator(const GeneratorDefinition* generator);

    constexpr void* generator_state(void) const;

    constexpr std::vector<ClimateSeed>& climate_seeds(void);
    constexpr const std::vector<ClimateSeed>& climate_seeds(void) const;

    ClimateSample sample_climate_block(const BlockPosXZ& pos) const;
    ClimateSampleArray sample_climate_array(const ChunkPosXZ& pos) const;

    const CachedEntropy& sample_entropy(const ChunkPosXZ& pos);
    std::uint64_t entropy_at(const BlockPosXZ& pos);

    biome_id_type biome_at(const BlockPosXZ& pos);
    biome_id_type find_climate_biome(const ClimateSample& sample) const;

    std::shared_ptr<ScatterEntry> scatter_entry(const ChunkPosXZ& pos);

    void commit_placements(BlockStorage& storage, const ChunkPos& cpos);
    void commit_placements(BlockStorage& storage, const ChunkPos& cpos, std::span<const FeatureInstance> instances);

    void init_generation(std::shared_ptr<void> state);

private:
    struct EntropyEntry final {
        std::once_flag init_flag;
        CachedEntropy data;
    };

    std::shared_ptr<EntropyEntry> find_entropy_entry(const ChunkPosXZ& pos) const;
    std::shared_ptr<EntropyEntry> get_entropy_entry(const ChunkPosXZ& pos);

    std::optional<biome_id_type> find_biome(const BlockPosXZ& pos) const;
    void store_biome(const BlockPosXZ& pos, biome_id_type biome);

    std::shared_ptr<ScatterEntry> find_scatter(const ChunkPosXZ& pos) const;

    const std::vector<FeatureInstance>& collect_placements(const ChunkPosXZ& pos);

    const GeneratorDefinition* m_generator { nullptr };

    std::shared_ptr<void> m_generator_state;

    std::unique_ptr<NoiseCache2D_8x8> m_climate_temperature;
    std::unique_ptr<NoiseCache2D_8x8> m_climate_humidity;
    std::unique_ptr<NoiseCache2D_8x8> m_climate_continentalness;
    std::unique_ptr<NoiseCache2D_8x8> m_climate_erosion;
    std::unique_ptr<NoiseCache2D_8x8> m_climate_weirdness;
    std::vector<ClimateSeed> m_climate_seeds;

    std::uint64_t m_entropy_seed { 0 };
    mutable std::shared_mutex m_entropy_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<EntropyEntry>> m_entropy_cache;

    mutable std::shared_mutex m_biome_mutex;
    vx::hash_map<BlockPosXZ, biome_id_type> m_biome_cache;

    mutable std::shared_mutex m_scatter_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<ScatterEntry>> m_scatter_cache;

    std::vector<std::unique_ptr<PlacementRule>> m_placement_rules;
    std::size_t m_placement_extent { 0 };
};

constexpr const GeneratorDefinition* ServerWorld::generator(void) const
{
    return m_generator;
}

constexpr void* ServerWorld::generator_state(void) const
{
    return m_generator_state.get();
}

constexpr std::vector<ClimateSeed>& ServerWorld::climate_seeds(void)
{
    return m_climate_seeds;
}

constexpr const std::vector<ClimateSeed>& ServerWorld::climate_seeds(void) const
{
    return m_climate_seeds;
}

#endif /* B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71 */
