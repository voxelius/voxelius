#include "client/pch.hh"

#include "client/world/lod_manager.hh"

#include "core/cvar/boolean.hh"
#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/msg/log.hh"

#include "shared/cvar/server.hh"
#include "shared/world/chunk_lod.hh"
#include "shared/world/lod_tile.hh"

#include "client/camera.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/settings.hh"
#include "client/world/lod_chunk_renderer.hh"
#include "client/world/lod_tile_renderer.hh"

constexpr static std::size_t MAX_CHUNK_LODS = 4096;
constexpr static std::size_t MAX_TILES = 8192;
constexpr static std::int32_t LOD_Y_KEEP = 8;

static cvar::Boolean s_lod { true };
static cvar::Number<unsigned> s_lod_range { 128, 0, 128 };
static cvar::Number<unsigned> s_lod_debug { 0, 0, 1 };
static cvar::Boolean s_lod_other_realms { true };

static gui::CheckBox s_lod_gui;
static gui::SliderUnsigned s_lod_range_gui;
static gui::CheckBox s_lod_other_realms_gui;

static vx::hash_map<ChunkPos, LodChunkEntry> s_chunks;
static vx::hash_map<LodKey, LodTileEntry> s_tiles;
static std::uint64_t s_revision;

static bool tile_intersects_box(const LodKey& key, const ChunkAlignedBox& box)
{
    auto edge = lod::tile_edge_chunks(key.level);
    auto min_x = key.x * edge;
    auto max_x = min_x + edge;
    auto min_z = key.z * edge;
    auto max_z = min_z + edge;

    if(max_x <= box.min().x() || min_x > box.max().x()) {
        return false;
    }

    if(max_z <= box.min().z() || min_z > box.max().z()) {
        return false;
    }

    return true;
}

static float chunk_distance_sq(const ChunkPos& cpos)
{
    return (cpos - camera::chunk).cast<float>().squaredNorm();
}

// A full cache is a reason to forget the farthest chunk, not a reason to drop
// the incoming one: a silently dropped pyramid is a hole the server will not
// send again.
static void evict_farthest_chunk(void)
{
    ChunkPos victim = ChunkPos::Zero();
    auto best = -1.0f;
    auto found = false;

    for(const auto& it : s_chunks) {
        auto distance = chunk_distance_sq(it.first);

        if(distance > best) {
            best = distance;
            victim = it.first;
            found = true;
        }
    }

    if(found) {
        s_chunks.erase(victim);
        s_revision += 1;
    }
}

static void evict_farthest_tile(void)
{
    auto victim = LodKey {};
    auto best = -1.0f;
    auto found = false;

    for(const auto& it : s_tiles) {
        auto edge = static_cast<float>(lod::tile_edge_chunks(it.first.level));
        auto center = Eigen::Vector2f(static_cast<float>(it.first.x) * edge + edge * 0.5f - static_cast<float>(camera::chunk.x()),
            static_cast<float>(it.first.z) * edge + edge * 0.5f - static_cast<float>(camera::chunk.z()));
        auto distance = center.squaredNorm();

        if(distance > best) {
            best = distance;
            victim = it.first;
            found = true;
        }
    }

    if(found) {
        s_tiles.erase(victim);
        s_revision += 1;
    }
}

static cvar::status cmd_lod_stats(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    msg::log_info("lod: enabled={} range={} chunks={} tiles={} revision={}", lod_manager::enabled() ? 1 : 0, lod_manager::range_chunks(),
        s_chunks.size(), s_tiles.size(), s_revision);

    lod_chunk_renderer::log_stats();
    lod_tile_renderer::log_stats();

    return cvar::status::OK;
}

void lod_manager::init(void)
{
    auto& lod_binding = cvar::bind_var(s_lod, "r_lod");
    lod_binding.set_help("Draw far terrain LOD: 0 = off, 1 = on");
    lod_binding.set_flags(FCVAR_ARCHIVE);

    auto& lod_range_binding = cvar::bind_var(s_lod_range, "r_lod_range");
    lod_range_binding.set_help("Far terrain LOD range, in chunks");
    lod_range_binding.set_flags(FCVAR_ARCHIVE);

    auto& lod_debug_binding = cvar::bind_var(s_lod_debug, "r_lod_debug");
    lod_debug_binding.set_help("Color LOD cells by their level: 0 = off, 1 = on");

    auto& lod_other_realms_binding = cvar::bind_var(s_lod_other_realms, "r_lod_other_realms");
    lod_other_realms_binding.set_help("Draw LODs of realms other than the camera realm: 0 = off, 1 = on");
    lod_other_realms_binding.set_flags(FCVAR_ARCHIVE);

    s_lod_gui.bind("r_lod");
    settings::video_lod.add_child(s_lod_gui, 1);

    s_lod_range_gui.set_range(0, 128);
    s_lod_range_gui.bind("r_lod_range");
    settings::video_lod.add_child(s_lod_range_gui, 2);

    s_lod_other_realms_gui.bind("r_lod_other_realms");
    settings::video_lod.add_child(s_lod_other_realms_gui, 3);

    auto& stats_binding = cvar::bind_cmd("lod_stats");
    stats_binding.set_help("Print the far terrain LOD cache state");
    stats_binding.connect<&cmd_lod_stats>();

    s_chunks.clear();
    s_tiles.clear();
    s_revision += 1;
}

void lod_manager::shutdown(void)
{
    s_chunks.clear();
    s_tiles.clear();
    s_revision += 1;
}

void lod_manager::store_chunk(const ChunkPos& cpos, std::shared_ptr<const ChunkLod> lod)
{
    if(s_chunks.size() >= MAX_CHUNK_LODS && !s_chunks.contains(cpos)) {
        evict_farthest_chunk();
    }

    auto& entry = s_chunks[cpos];

    entry.data = std::move(lod);
    entry.instances.clear();
    entry.level = LOD_CHUNK_LEVEL_NONE;
    entry.generation += 1;
    entry.dirty = true;

    s_revision += 1;
}

void lod_manager::store_tile(const LodKey& key, std::shared_ptr<const LodTile> tile)
{
    if(s_tiles.size() >= MAX_TILES && !s_tiles.contains(key)) {
        evict_farthest_tile();
    }

    auto& entry = s_tiles[key];

    entry.data = std::move(tile);

    s_revision += 1;
}

void lod_manager::evict_outside(const ChunkAlignedBox& lod_box)
{
    auto changed = false;
    auto start = static_cast<std::int32_t>(std::max(camera::view_distance.value(), 1U));
    auto end = start + std::max<std::int32_t>(1, start / 4);

    for(auto it = s_chunks.begin(); it != s_chunks.end();) {
        auto delta = (it->first - camera::chunk).cwiseAbs();
        auto distance = delta.maxCoeff();

        auto inside = it->first.x() >= lod_box.min().x() && it->first.x() <= lod_box.max().x();
        inside = inside && it->first.z() >= lod_box.min().z() && it->first.z() <= lod_box.max().z();
        inside = inside && delta.y() <= LOD_Y_KEEP;
        inside = inside && distance < end;

        if(inside) {
            it = std::next(it);
        }
        else {
            it = s_chunks.erase(it);
            changed = true;
        }
    }

    for(auto it = s_tiles.begin(); it != s_tiles.end();) {
        if(tile_intersects_box(it->first, lod_box)) {
            it = std::next(it);
        }
        else {
            it = s_tiles.erase(it);
            changed = true;
        }
    }

    if(changed) {
        s_revision += 1;
    }
}

bool lod_manager::enabled(void)
{
    return s_lod.value() && s_lod_range.value() > 0 && server_vars::lod_distance.value() > 0;
}

bool lod_manager::debug_levels(void)
{
    return static_cast<bool>(s_lod_debug.value());
}

bool lod_manager::other_realms(void)
{
    return s_lod_other_realms.value();
}

std::int32_t lod_manager::range_chunks(void)
{
    if(!enabled()) {
        return 0;
    }

    return std::min<std::int32_t>(static_cast<std::int32_t>(s_lod_range.value()),
        static_cast<std::int32_t>(server_vars::lod_distance.value()));
}

vx::hash_map<ChunkPos, LodChunkEntry>& lod_manager::chunks(void)
{
    return s_chunks;
}

std::vector<std::pair<LodKey, std::shared_ptr<const LodTile>>> lod_manager::snapshot_tiles(void)
{
    std::vector<std::pair<LodKey, std::shared_ptr<const LodTile>>> result;
    result.reserve(s_tiles.size());

    for(const auto& it : s_tiles) {
        result.emplace_back(it.first, it.second.data);
    }

    return result;
}

std::uint64_t lod_manager::mesh_revision(void)
{
    return s_revision;
}

void lod_manager::notify_mesh_changed(void)
{
    s_revision += 1;
}
