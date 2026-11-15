#include "server/pch.hh"

#include "server/world/worldgen.hh"

#include "core/threading.hh"

#include "shared/globals.hh"
#include "shared/utils/biome.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "server/universe.hh"
#include "server/world/climate_noise.hh"
#include "server/world/entropy_cache.hh"
#include "server/world/feature_placer.hh"
#include "server/world/feature_scatter.hh"
#include "server/world/heightmap.hh"
#include "server/world/realm_sky.hh"
#include "server/world/realm_surface.hh"
#include "server/world/terrain.hh"

class WorldgenTask final : public Task {
public:
    explicit WorldgenTask(const ChunkPos& pos);
    virtual ~WorldgenTask(void) override;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    BlockStorage m_blocks;
    std::unique_ptr<BiomeSlice> m_biomes;
    ChunkPos m_pos;
};

static vx::hash_set<ChunkPos> s_pending;

WorldgenTask::WorldgenTask(const ChunkPos& pos) : m_pos(pos)
{
    m_blocks.fill(BLOCK_ID_NULL);
}

WorldgenTask::~WorldgenTask(void)
{
    s_pending.erase(m_pos);
}

void WorldgenTask::process(void)
{
    ZoneScopedN("worldgen::process");

    auto realm = utils::realm(m_pos.y());

    if(realm != BIOME_REALM_SURFACE && realm != BIOME_REALM_SKY) {
        status.store(task_status::CANCELLED, std::memory_order_release);
        return;
    }

    auto biomes = std::make_unique<BiomeSlice>();

    if(!terrain::generate(m_pos, m_blocks, *biomes)) {
        status.store(task_status::CANCELLED, std::memory_order_release);
        return;
    }

    m_biomes = std::move(biomes);
}

void WorldgenTask::finalize(void)
{
    ZoneScopedN("worldgen::finalize");

    auto chunk = world::create_chunk(m_pos);
    chunk->set_blocks(std::move(m_blocks));

    if(m_biomes) {
        auto has_biome = std::any_of(m_biomes->cbegin(), m_biomes->cend(), [](biome_id_type id) {
            return static_cast<bool>(id);
        });

        if(has_biome) {
            auto realm = utils::realm(m_pos.y());
            auto cpos_xz = ChunkPosXZ(m_pos.x(), m_pos.z());
            biome_map::insert(realm, cpos_xz, std::move(*m_biomes));
        }

        m_biomes.reset();
    }

    globals::dispatcher.trigger(ChunkUpdateEvent(m_pos, chunk));
    globals::dispatcher.trigger(ChunkReadyEvent(m_pos, chunk));
}

void worldgen::init(void)
{
    std::mt19937_64 seeder;
    seeder.seed(universe::seed());

    entropy_cache::init(seeder);
    climate_noise::init(seeder);

    realm_surface::init(seeder);
    realm_sky::init(seeder);

    feature_placer::init();

    FeatureScatter::init();
}

void worldgen::shutdown(void)
{
    // Pending generators still touch feature rules, noise caches and the
    // heightmap. Drain them before any of that is destroyed, otherwise a
    // worker can outlive the teardown and keep feature handles alive (or
    // use the destroyed state outright).
    threading::wait();

    feature_placer::shutdown();

    realm_sky::shutdown();
    realm_surface::shutdown();

    climate_noise::shutdown();
    heightmap::purge();
    entropy_cache::shutdown();
}

void worldgen::request(const ChunkPos& pos)
{
    if(s_pending.contains(pos)) {
        return;
    }

    s_pending.emplace(pos);

    threading::submit<WorldgenTask>(pos);
}

bool worldgen::pending(const ChunkPos& pos)
{
    return s_pending.contains(pos);
}
