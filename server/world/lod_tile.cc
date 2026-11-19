#include "server/pch.hh"

#include "server/world/lod_tile.hh"

#include "core/buffer.hh"
#include "core/threading.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/lod.hh"
#include "shared/world/lod_tile.hh"

#include "server/universe.hh"
#include "server/world/heightmap.hh"
#include "server/world/lod_color.hh"

constexpr static std::size_t MAX_CACHE = 4096;
constexpr static std::size_t MAX_QUEUED = 512;
constexpr static std::size_t MAX_TASKS_PER_TICK = 16;
static const Eigen::Vector3f DEFAULT_WATER_COLOR { 0.25f, 0.45f, 0.85f };
static const Eigen::Vector3f DEFAULT_GROUND_COLOR { 0.5f, 0.5f, 0.5f };

struct RawTileCell final {
    Eigen::Vector3f color;
    std::int16_t height;
    std::int16_t bottom;
    std::uint8_t flags;
};

class LodTileTask final : public Task {
public:
    explicit LodTileTask(const LodKey& key);
    virtual ~LodTileTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    LodKey m_key;
    std::shared_ptr<LodTile> m_result;
};

static std::mutex s_cache_mutex;
static vx::hash_map<LodKey, std::shared_ptr<const LodTile>> s_cache;

static std::mutex s_waiting_mutex;
static vx::hash_map<LodKey, std::vector<ENetPeer*>> s_waiting;
static vx::hash_set<LodKey> s_pending;

static std::mutex s_jobs_mutex;
static std::deque<LodKey> s_jobs;

static std::filesystem::path tile_path(const LodKey& key)
{
    auto ux = std::bit_cast<std::uint32_t>(key.x);
    auto uz = std::bit_cast<std::uint32_t>(key.z);
    auto name = std::format("R{}L{}X{:08X}Z{:08X}.zlod", static_cast<unsigned>(key.realm), static_cast<unsigned>(key.level), ux, uz);
    return universe::lod_dir() / "tile" / name;
}

static std::shared_ptr<const LodTile> find_cached(const LodKey& key)
{
    std::scoped_lock lock(s_cache_mutex);

    auto it = s_cache.find(key);

    if(it == s_cache.end()) {
        return nullptr;
    }

    return it->second;
}

static void store_cached(const LodKey& key, std::shared_ptr<const LodTile> tile)
{
    std::scoped_lock lock(s_cache_mutex);

    if(s_cache.size() >= MAX_CACHE && !s_cache.contains(key)) {
        if(!s_cache.empty()) {
            s_cache.erase(s_cache.begin()->first);
        }
    }

    s_cache.insert_or_assign(key, std::move(tile));
}

static bool read_file(const LodKey& key, LodTile& tile)
{
    std::ifstream stream(tile_path(key), std::ios::in | std::ios::binary);

    if(!stream.is_open()) {
        return false;
    }

    std::vector<char> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    if(data.empty()) {
        return false;
    }

    ReadBuffer buffer;
    buffer.reset(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size()));

    constexpr auto header_size = sizeof(std::uint8_t) + sizeof(std::uint64_t) * 2;

    if(!buffer.has_records(1, header_size)) {
        return false;
    }

    if(buffer.read<std::uint8_t>() != LOD_TILE_VERSION) {
        return false;
    }

    if(buffer.read<std::uint64_t>() != universe::seed()) {
        return false;
    }

    if(buffer.read<std::uint64_t>() != block_registry::checksum()) {
        return false;
    }

    if(!LodTile::decode(tile, buffer)) {
        return false;
    }

    tile.key = key;
    return true;
}

static void write_file(const LodKey& key, const LodTile& tile)
{
    auto path = tile_path(key);

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    WriteBuffer buffer;
    buffer.write<std::uint8_t>(LOD_TILE_VERSION);
    buffer.write<std::uint64_t>(universe::seed());

    // The colours are derived from the block and biome registries. Their
    // checksums are still placeholders, so a content change that only moves
    // colours needs LOD_TILE_VERSION bumped and save/lod/tile wiped.
    buffer.write<std::uint64_t>(block_registry::checksum());
    LodTile::encode(tile, buffer);

    auto temporary = path;
    temporary += ".tmp";

    std::ofstream stream(temporary, std::ios::out | std::ios::binary | std::ios::trunc);

    if(!stream.is_open()) {
        return;
    }

    buffer.to_stream(stream);
    stream.close();

    std::filesystem::remove(path, ec);
    std::filesystem::rename(temporary, path, ec);
}

static bool sample_cell(const LodKey& key, std::int32_t cell_x, std::int32_t cell_z, RawTileCell& out)
{
    auto cell_size = lod::tile_cell_size(key.level);
    auto edge_blocks = lod::tile_edge_blocks(key.level);
    auto origin_x = static_cast<BlockPosXZ::value_type>(key.x) * edge_blocks;
    auto origin_z = static_cast<BlockPosXZ::value_type>(key.z) * edge_blocks;

    auto base_x = origin_x + static_cast<BlockPosXZ::value_type>(cell_x) * cell_size;
    auto base_z = origin_z + static_cast<BlockPosXZ::value_type>(cell_z) * cell_size;

    auto sample_count = key.level >= 3 ? 2 : 1;
    auto best_height = std::numeric_limits<BlockPos::value_type>::min();
    auto found = false;

    ColumnProbe best {};

    for(std::int32_t sz = 0; sz < sample_count; sz += 1) {
        for(std::int32_t sx = 0; sx < sample_count; sx += 1) {
            auto offset = cell_size / (sample_count * 2);
            auto sample_x = base_x + offset + static_cast<BlockPosXZ::value_type>(sx) * (cell_size / sample_count);
            auto sample_z = base_z + offset + static_cast<BlockPosXZ::value_type>(sz) * (cell_size / sample_count);

            auto probe = heightmap::probe_column(key.realm, BlockPosXZ(sample_x, sample_z));
            auto height = std::numeric_limits<BlockPos::value_type>::min();

            if(probe.column.liquid_y != Column::UNSET) {
                height = probe.column.liquid_y + 1;
            }
            else if(probe.column.surface_y != Column::UNSET) {
                height = probe.column.surface_y + 1;
            }

            if(height > best_height) {
                best_height = height;
                best = probe;
                found = true;
            }
        }
    }

    if(!found) {
        out.color = Eigen::Vector3f::Zero();
        out.height = LOD_CELL_EMPTY;
        out.bottom = LOD_CELL_EMPTY;
        out.flags = 0;
        return false;
    }

    auto biome_def = biome_registry::find_definition(best.biome);
    auto water = best.column.liquid_y != Column::UNSET;
    auto block = BLOCK_ID_NULL;

    if(water) {
        if(biome_def) {
            block = biome_def->palette_fluid.cached;
        }
    }
    else if(biome_def) {
        block = biome_def->palette_surface.cached;
    }

    auto color = water ? DEFAULT_WATER_COLOR : DEFAULT_GROUND_COLOR;

    if(block) {
        color = lod_color::block_color(block, best.biome);
    }

    out.color = color;
    out.height = static_cast<std::int16_t>(std::clamp<BlockPos::value_type>(best_height, INT16_MIN, INT16_MAX));

    // A surface column is solid all the way down, so the tile only needs a
    // one-block skirt to close the seam against a neighbour tile. Keeping the
    // whole cell as the skirt drew walls taller than the terrain at coarse
    // levels, which showed up as lines of blocks along every tile border.
    auto bottom = best_height - 1;

    if(best.column.bottom_y != Column::UNSET) {
        bottom = best.column.bottom_y;
    }

    out.bottom = static_cast<std::int16_t>(std::clamp<BlockPos::value_type>(bottom, INT16_MIN, INT16_MAX));
    out.flags = water ? LOD_CELL_WATER : 0;

    return true;
}

static bool build_tile(const LodKey& key, LodTile& tile)
{
    thread_local std::array<RawTileCell, LOD_TILE_CELL_COUNT> raw;

    std::fill(raw.begin(), raw.end(), RawTileCell { Eigen::Vector3f::Zero(), LOD_CELL_EMPTY, LOD_CELL_EMPTY, 0 });

    for(std::int32_t z = 0; z < static_cast<std::int32_t>(LOD_TILE_CELLS); ++z) {
        for(std::int32_t x = 0; x < static_cast<std::int32_t>(LOD_TILE_CELLS); ++x) {
            auto index = static_cast<std::size_t>(z * LOD_TILE_CELLS + x);
            sample_cell(key, x, z, raw[index]);
        }
    }

    vx::hash_set<std::uint32_t> unique;
    auto overflow = false;

    for(const auto& cell : raw) {
        if(cell.height == LOD_CELL_EMPTY) {
            continue;
        }

        auto rgb = lod::to_rgb8(cell.color);
        std::uint32_t key_value = 0;
        key_value |= static_cast<std::uint32_t>(rgb[0]) << 16U;
        key_value |= static_cast<std::uint32_t>(rgb[1]) << 8U;
        key_value |= static_cast<std::uint32_t>(rgb[2]);

        if(unique.insert(key_value).second && unique.size() > lod::PALETTE_MAX) {
            overflow = true;
            break;
        }
    }

    tile.key = key;
    tile.color_mode = overflow ? lod::COLOR_RGB332 : lod::COLOR_PALETTE8;
    tile.palette.clear();

    vx::hash_map<std::uint32_t, std::uint8_t> indices;

    for(std::size_t i = 0; i < LOD_TILE_CELL_COUNT; ++i) {
        auto& source = raw[i];
        auto& dest = tile.cells[i];

        dest.height = source.height;
        dest.bottom = source.bottom;
        dest.flags = source.flags;

        if(source.height == LOD_CELL_EMPTY) {
            dest.color = 0;
            continue;
        }

        if(overflow) {
            dest.color = lod::to_rgb332(source.color);
            continue;
        }

        auto rgb = lod::to_rgb8(source.color);
        std::uint32_t key_value = 0;
        key_value |= static_cast<std::uint32_t>(rgb[0]) << 16U;
        key_value |= static_cast<std::uint32_t>(rgb[1]) << 8U;
        key_value |= static_cast<std::uint32_t>(rgb[2]);

        auto it = indices.find(key_value);
        std::uint8_t color = 0;

        if(it == indices.end()) {
            color = static_cast<std::uint8_t>(tile.palette.size());
            tile.palette.push_back(rgb);
            indices[key_value] = color;
        }
        else {
            color = it->second;
        }

        dest.color = color;
    }

    return true;
}

static void send_tile(const LodKey& key, const LodTile& tile, ENetPeer* peer)
{
    packet::World_LodTile packet;
    packet.key = key;
    packet.tile = tile;
    protocol::send(packet, peer);
}

static void flush_waiting(const LodKey& key, const std::shared_ptr<const LodTile>& tile)
{
    std::scoped_lock lock(s_waiting_mutex);

    auto it = s_waiting.find(key);

    if(it == s_waiting.end()) {
        return;
    }

    for(auto peer : it->second) {
        send_tile(key, *tile, peer);
    }

    s_waiting.erase(it);
}

LodTileTask::LodTileTask(const LodKey& key) : m_key(key)
{
    // empty
}

void LodTileTask::process(void)
{
    ZoneScopedN("lod_tile::process");

    auto loaded = std::make_shared<LodTile>();

    if(read_file(m_key, *loaded)) {
        m_result = std::move(loaded);
        return;
    }

    auto built = std::make_shared<LodTile>();

    if(!build_tile(m_key, *built)) {
        return;
    }

    write_file(m_key, *built);
    m_result = std::move(built);
}

static void forget_pending(const LodKey& key)
{
    std::scoped_lock lock(s_jobs_mutex);
    s_pending.erase(key);
}

static void clear_jobs(void)
{
    std::scoped_lock lock(s_jobs_mutex);
    s_jobs.clear();
    s_pending.clear();
}

static void clear_waiting(void)
{
    std::scoped_lock lock(s_waiting_mutex);
    s_waiting.clear();
}

static void clear_cache(void)
{
    std::scoped_lock lock(s_cache_mutex);
    s_cache.clear();
}

static bool take_job(LodKey& key)
{
    std::scoped_lock lock(s_jobs_mutex);

    if(s_jobs.empty()) {
        return false;
    }

    key = s_jobs.front();
    s_jobs.pop_front();
    return true;
}

static void add_waiter(const LodKey& key, ENetPeer* peer)
{
    std::scoped_lock lock(s_waiting_mutex);

    auto& waiting = s_waiting[key];

    if(std::find(waiting.begin(), waiting.end(), peer) == waiting.end()) {
        waiting.push_back(peer);
    }
}

void LodTileTask::finalize(void)
{
    ZoneScopedN("lod_tile::finalize");

    forget_pending(m_key);

    if(m_result == nullptr) {
        return;
    }

    store_cached(m_key, m_result);
    flush_waiting(m_key, m_result);
}

void lod_tile::init(void)
{
    // empty
}

void lod_tile::shutdown(void)
{
    // A build still on the pool samples the heightmap and the LOD palette.
    // Drain it before the callers take either of them apart.
    threading::wait();

    clear_jobs();
    clear_waiting();
    clear_cache();
}

void lod_tile::fixed_update(void)
{
    ZoneScoped;

    for(std::size_t i = 0; i < MAX_TASKS_PER_TICK; ++i) {
        LodKey key {};

        if(!take_job(key)) {
            break;
        }

        threading::submit<LodTileTask>(key);
    }
}

bool lod_tile::request(const LodKey& key, ENetPeer* peer)
{
    if(auto cached = find_cached(key)) {
        send_tile(key, *cached, peer);
        return true;
    }

    add_waiter(key, peer);

    std::scoped_lock lock(s_jobs_mutex);

    if(s_pending.contains(key)) {
        return true;
    }

    if(s_jobs.size() >= MAX_QUEUED) {
        // The peer stays in the waiting list, so the request is retried on
        // the next sweep. Say it once: a silent cap looks like a world hole.
        static std::atomic_bool s_warned;
        auto expected = false;

        if(s_warned.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
            msg::log_warning("lod_tile: build queue is full, far tiles fall behind");
        }

        return false;
    }

    s_pending.insert(key);
    s_jobs.push_back(key);
    return true;
}

void lod_tile::forget_peer(ENetPeer* peer)
{
    std::scoped_lock lock(s_waiting_mutex);

    for(auto it = s_waiting.begin(); it != s_waiting.end();) {
        auto& peers = it->second;

        peers.erase(std::remove(peers.begin(), peers.end(), peer), peers.end());

        if(peers.empty()) {
            it = s_waiting.erase(it);
        }
        else {
            it = std::next(it);
        }
    }
}

std::size_t lod_tile::backlog(void)
{
    std::scoped_lock lock(s_jobs_mutex);
    return s_jobs.size() + s_pending.size();
}
