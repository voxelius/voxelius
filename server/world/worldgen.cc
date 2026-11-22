#include "server/pch.hh"

#include "server/world/worldgen.hh"

#include "core/threading.hh"

#include "shared/globals.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "server/world/generator_map.hh"
#include "server/world/world.hh"

class WorldgenTask final : public Task {
public:
    WorldgenTask(std::shared_ptr<World> world, const ChunkPos& pos);
    virtual ~WorldgenTask(void) override;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::shared_ptr<World> m_world;
    BlockStorage m_blocks;
    std::unique_ptr<BiomeSlice> m_biomes;
    ChunkPos m_pos;
};

static vx::hash_map<const World*, vx::hash_set<ChunkPos>> s_pending;

WorldgenTask::WorldgenTask(std::shared_ptr<World> world, const ChunkPos& pos) : m_world(std::move(world)), m_pos(pos)
{
    m_blocks.fill(BLOCK_ID_NULL);
}

WorldgenTask::~WorldgenTask(void)
{
    s_pending[m_world.get()].erase(m_pos);
}

void WorldgenTask::process(void)
{
    ZoneScopedN("worldgen::process");

    auto biomes = std::make_unique<BiomeSlice>();

    auto world = std::static_pointer_cast<ServerWorld>(m_world);
    auto generator = world->generator();

    if(generator == nullptr) {
        status.store(task_status::CANCELLED, std::memory_order_release);
        return;
    }

    if(!generator->generate_blocks(world.get(), m_pos, m_blocks, *biomes)) {
        status.store(task_status::CANCELLED, std::memory_order_release);
        return;
    }

    m_biomes = std::move(biomes);
}

void WorldgenTask::finalize(void)
{
    ZoneScopedN("worldgen::finalize");

    auto chunk = m_world->create_chunk(m_pos);
    chunk->set_blocks(std::move(m_blocks));

    if(m_biomes) {
        auto has_biome = std::any_of(m_biomes->cbegin(), m_biomes->cend(), [](biome_id_type id) {
            return static_cast<bool>(id);
        });

        if(has_biome) {
            auto cpos_xz = ChunkPosXZ(m_pos.x(), m_pos.z());
            m_world->insert_biomes(cpos_xz, std::move(*m_biomes));
        }

        m_biomes.reset();
    }

    globals::dispatcher.trigger(ChunkUpdateEvent(m_world.get(), m_pos, chunk));
    globals::dispatcher.trigger(ChunkReadyEvent(m_world.get(), m_pos, chunk));
}

void worldgen::init(void)
{
    // empty
}

void worldgen::shutdown(void)
{
    // Pending generators still touch feature rules, noise caches and the heightmap.
    // Drain them before any of that is destroyed, otherwise a worker can outlive the
    // teardown and keep feature handles alive (or use the destroyed state outright).
    threading::wait();
}

void worldgen::request(const std::shared_ptr<World>& world, const ChunkPos& pos)
{
    auto& pending = s_pending[world.get()];

    if(pending.contains(pos)) {
        return;
    }

    pending.emplace(pos);

    threading::submit<WorldgenTask>(world, pos);
}

bool worldgen::pending(World* world, const ChunkPos& pos)
{
    assert(world);
    return s_pending[world].contains(pos);
}
