#include "server/pch.hh"

#include "server/world/chunk_loader.hh"

#include "core/buffer.hh"
#include "core/threading.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/biome.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "server/entity/entity_loader.hh"
#include "server/universe.hh"
#include "server/utils/region.hh"
#include "server/world/worldgen.hh"
#include "server/world/zvox_file.hh"

constexpr static std::size_t MAX_REGIONS = 32;

struct RegionEntry final {
    explicit RegionEntry(std::filesystem::path path);

    std::mutex io_mutex;
    ZvoxFile region_file;
    std::atomic<std::chrono::steady_clock::time_point> last_used;
};

class LoadTask final : public Task {
public:
    LoadTask(std::shared_ptr<World> world, ChunkPos pos);
    virtual ~LoadTask(void) override;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::shared_ptr<World> m_world;
    ChunkPos m_pos;
    Chunk::Snapshot m_snapshot;
    bool m_found { false };
};

class SaveTask final : public Task {
public:
    SaveTask(std::shared_ptr<World> world, ChunkPos pos, Chunk::Snapshot snapshot);
    virtual ~SaveTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::shared_ptr<World> m_world;
    ChunkPos m_pos;
    Chunk::Snapshot m_snapshot;
};

static std::shared_mutex s_cache_mutex;
static vx::hash_map<RegionKey, std::shared_ptr<RegionEntry>> s_cache;
static vx::hash_map<const World*, vx::hash_set<ChunkPos>> s_pending;
static vx::hash_map<const World*, vx::hash_set<ChunkPos>> s_saving;
static vx::hash_map<const World*, vx::hash_set<ChunkPos>> s_deferred;

static std::filesystem::path region_path(const World* world, const RegionPos& region)
{
    auto ux = std::bit_cast<std::uint32_t>(region.x());
    auto uy = std::bit_cast<std::uint32_t>(region.y());
    auto uz = std::bit_cast<std::uint32_t>(region.z());
    auto name = std::format("R{:08X}{:08X}{:08X}.zvox", ux, uy, uz);
    return world->chunk_dir() / name;
}

static std::shared_ptr<RegionEntry> find_region(const World* world, const RegionPos& pos)
{
    RegionKey key {};
    key.world = world;
    key.pos = pos;

    std::shared_lock lock(s_cache_mutex);

    auto it = s_cache.find(key);

    if(it == s_cache.cend()) {
        return nullptr;
    }

    it->second->last_used.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    return it->second;
}

static std::shared_ptr<RegionEntry> get_region(const World* world, const RegionPos& pos)
{
    if(auto entry = find_region(world, pos)) {
        return entry;
    }

    RegionKey key {};
    key.world = world;
    key.pos = pos;

    std::unique_lock lock(s_cache_mutex);

    // Another thread may have opened the region while this one waited for
    // the exclusive lock. Opening the same file twice would give both
    // instances their own stream and free-range bookkeeping, and their
    // writes would corrupt each other.
    if(auto it = s_cache.find(key); it != s_cache.cend()) {
        it->second->last_used.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
        return it->second;
    }

    auto entry = std::make_shared<RegionEntry>(region_path(world, pos));
    entry->last_used.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    s_cache.insert_or_assign(key, std::shared_ptr(entry));

    return entry;
}

static void evict_stale(void)
{
    std::unique_lock lock(s_cache_mutex);

    if(s_cache.size() > MAX_REGIONS) {
        std::vector<std::pair<RegionKey, std::shared_ptr<RegionEntry>>> entries;
        entries.reserve(s_cache.size());

        for(const auto& it : s_cache) {
            entries.emplace_back(it.first, it.second);
        }

        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            return a.second->last_used.load(std::memory_order_relaxed) < b.second->last_used.load(std::memory_order_relaxed);
        });

        for(const auto& [key, entry] : entries) {
            if(s_cache.size() <= MAX_REGIONS) {
                break;
            }

            if(entry.use_count() <= 2) {
                // Baseline while iterating this snapshot is 2 refs
                // (s_cache's own + this local copy); anything above
                // that means an in-flight task is still using it.
                s_cache.erase(key);
            }
        }
    }
}

static void write_chunk(const World* world, const ChunkPos& pos, const Chunk::Snapshot& snapshot)
{
    auto region = utils::region_of(pos);
    auto slot = utils::region_slot(pos, region);

    auto entry = get_region(world, region);

    WriteBuffer buffer;
    Chunk::encode_dat(snapshot, buffer);

    std::scoped_lock io_lock(entry->io_mutex);
    entry->region_file.write_slot(slot, buffer);
}

static void finish_save(const std::shared_ptr<World>& world, const ChunkPos& pos)
{
    s_saving[world.get()].erase(pos);

    if(s_deferred[world.get()].erase(pos)) {
        chunk_loader::request(world, pos);
    }
}

static void on_chunk_update(const ChunkUpdateEvent& event)
{
    auto world = event.world();
    auto& chunk_registry = world->chunk_registry();
    auto chunk = event.chunk();

    chunk_registry.emplace_or_replace<ChunkDirtyMarker>(chunk->entity());
}

static void on_block_update(const BlockUpdateEvent& event)
{
    auto world = event.world();
    auto& chunk_registry = world->chunk_registry();
    auto chunk = event.chunk();

    chunk_registry.emplace_or_replace<ChunkDirtyMarker>(chunk->entity());
}

static void on_chunk_light(const ChunkLightUpdateEvent& event)
{
    // The light lives in the chunk snapshot, so a chunk whose light changed
    // has to be saved even when its blocks did not.
    auto world = event.world();
    auto chunk = world->find_chunk(event.pos());

    if(chunk) {
        auto& chunk_registry = world->chunk_registry();
        chunk_registry.emplace_or_replace<ChunkDirtyMarker>(chunk->entity());
    }
}

RegionEntry::RegionEntry(std::filesystem::path path) : region_file(std::move(path))
{
    // empty
}

LoadTask::LoadTask(std::shared_ptr<World> world, ChunkPos pos) : m_world(std::move(world)), m_pos(std::move(pos))
{
    // empty
}

LoadTask::~LoadTask(void)
{
    s_pending[m_world.get()].erase(m_pos);
}

static bool read_region_slot(RegionEntry& entry, std::size_t slot, ReadBuffer& buffer)
{
    std::scoped_lock io_lock(entry.io_mutex);
    return entry.region_file.read_slot(slot, buffer);
}

void LoadTask::process(void)
{
    ZoneScopedN("chunk_loader::load");

    auto region = utils::region_of(m_pos);
    auto slot = utils::region_slot(m_pos, region);

    auto entry = get_region(m_world.get(), region);

    ReadBuffer buffer;
    auto found = false;

    found = read_region_slot(*entry, slot, buffer);
    if(found) {
        Chunk::decode_dat(m_snapshot, buffer);
        m_found = true;
    }
}

void LoadTask::finalize(void)
{
    ZoneScopedN("chunk_loader::load_finalize");

    if(m_found) {
        auto chunk = m_world->create_chunk(m_pos);
        chunk->restore(std::move(m_snapshot), globals::current_tick);

        // Not ChunkUpdateEvent: loaded-from-disk chunks are identical
        // to what's saved and must NOT be marked ChunkDirtyMarker.
        globals::dispatcher.trigger(ChunkReadyEvent(m_world.get(), m_pos, chunk));

        entity_loader::load(m_world.get(), m_pos);
    }
    else {
        worldgen::request(m_world, m_pos);
    }
}

SaveTask::SaveTask(std::shared_ptr<World> world, ChunkPos pos, Chunk::Snapshot snapshot)
    : m_world(std::move(world)), m_pos(std::move(pos)), m_snapshot(std::move(snapshot))
{
    // empty
}

void SaveTask::process(void)
{
    ZoneScopedN("chunk_loader::save");

    write_chunk(m_world.get(), m_pos, m_snapshot);
}

void SaveTask::finalize(void)
{
    finish_save(m_world, m_pos);
}

void chunk_loader::init(void)
{
    globals::dispatcher.sink<ChunkUpdateEvent>().connect<&on_chunk_update>();
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
    globals::dispatcher.sink<ChunkLightUpdateEvent>().connect<&on_chunk_light>();
}

void chunk_loader::shutdown(void)
{
    // Unloading submits saves to the pool; without this they are still
    // queued when the pool is cancelled and the chunks are lost.
    threading::wait();

    for(const auto& world : universe::worlds()) {
        auto& chunk_registry = world->chunk_registry();
        auto view = chunk_registry.view<Chunk_Component, ChunkDirtyMarker>();

        for(auto [entity, component] : view.each()) {
            write_chunk(world.get(), component.position, component.ptr->snapshot(globals::current_tick));
        }
    }

    std::unique_lock lock(s_cache_mutex);
    s_cache.clear();
}

void chunk_loader::fixed_update(void)
{
    evict_stale();
}

void chunk_loader::request(const std::shared_ptr<World>& world, const ChunkPos& pos)
{
    assert(world);

    if(world->find_chunk(pos)) {
        return;
    }

    auto& pending = s_pending[world.get()];

    if(pending.contains(pos)) {
        return;
    }

    auto& saving = s_saving[world.get()];

    if(saving.contains(pos)) {
        s_deferred[world.get()].emplace(pos);
        return;
    }

    pending.emplace(pos);

    threading::submit<LoadTask>(world, pos);
}

bool chunk_loader::pending(World* world, const ChunkPos& pos)
{
    assert(world);

    return s_pending[world].contains(pos) || s_saving[world].contains(pos);
}

bool chunk_loader::save(const std::shared_ptr<World>& world, const ChunkPos& pos)
{
    assert(world);

    auto chunk = world->find_chunk(pos);

    if(chunk == nullptr) {
        return false;
    }

    auto entity = chunk->entity();
    auto& chunk_registry = world->chunk_registry();

    if(chunk_registry.all_of<ChunkDirtyMarker>(entity)) {
        chunk_registry.remove<ChunkDirtyMarker>(entity);
        s_saving[world.get()].emplace(pos);
        threading::submit<SaveTask>(world, pos, chunk->snapshot(globals::current_tick));
        return true;
    }

    return false;
}
