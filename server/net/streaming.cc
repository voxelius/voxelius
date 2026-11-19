#include "server/pch.hh"

#include "server/net/streaming.hh"

#include "shared/component/transform.hh"
#include "shared/constant.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/view.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/receive.hh"
#include "server/net/sessions.hh"
#include "server/world/chunk_lod.hh"
#include "server/world/lod_tile.hh"

constexpr static std::int32_t LOD_Y_RADIUS = 2;

// The chunk LOD ring starts this many chunks inside the full view. The
// client hides a pyramid under a full chunk that already has a mesh, so
// the overlap closes the seam where the full chunk has not arrived yet.
constexpr static std::int32_t LOD_RING_OVERLAP = 2;
constexpr static std::uint16_t MAX_CHUNK_BUDGET = 256;
constexpr static std::uint16_t MAX_LOD_CHUNK_BUDGET = 64;
constexpr static std::uint16_t MAX_LOD_TILE_BUDGET = 16;

static ChunkAlignedBox make_empty_box(void)
{
    return ChunkAlignedBox(ChunkPos::Ones(), ChunkPos::Zero());
}

static std::int32_t iabs(std::int32_t value)
{
    return value < 0 ? -value : value;
}

static bool box_is_valid(const ChunkAlignedBox& box)
{
    auto valid = true;
    valid = valid && box.min().x() <= box.max().x();
    valid = valid && box.min().y() <= box.max().y();
    valid = valid && box.min().z() <= box.max().z();
    return valid;
}

static bool box_is_same(const ChunkAlignedBox& a, const ChunkAlignedBox& b)
{
    auto same_min = (a.min().array() == b.min().array()).all();
    auto same_max = (a.max().array() == b.max().array()).all();
    return same_min && same_max;
}

static ChunkAlignedBox intersect_boxes(const ChunkAlignedBox& a, const ChunkAlignedBox& b)
{
    auto min = a.min().cwiseMax(b.min());
    auto max = a.max().cwiseMin(b.max());
    return ChunkAlignedBox(min, max);
}

static std::int32_t floor_log2(std::int32_t value)
{
    auto unsigned_value = static_cast<unsigned>(std::max(value, 1));
    return static_cast<std::int32_t>(std::bit_width(unsigned_value)) - 1;
}

static std::int32_t floor_div(std::int32_t value, std::int32_t divisor)
{
    auto quotient = value / divisor;
    auto remainder = value % divisor;

    if(remainder && ((remainder < 0) != (divisor < 0))) {
        quotient -= 1;
    }

    return quotient;
}

static std::int32_t view_radius(const ChunkAlignedBox& view, const ChunkPos& player)
{
    auto radius = std::int32_t { 0 };
    radius = std::max(radius, iabs(view.min().x() - player.x()));
    radius = std::max(radius, iabs(view.max().x() - player.x()));
    radius = std::max(radius, iabs(view.min().z() - player.z()));
    radius = std::max(radius, iabs(view.max().z() - player.z()));
    return radius;
}

static std::int32_t lod_radius(const ChunkAlignedBox& lod, const ChunkPos& player)
{
    auto radius = std::int32_t { 0 };
    radius = std::max(radius, iabs(lod.min().x() - player.x()));
    radius = std::max(radius, iabs(lod.max().x() - player.x()));
    radius = std::max(radius, iabs(lod.min().z() - player.z()));
    radius = std::max(radius, iabs(lod.max().z() - player.z()));
    return radius;
}

static bool realm_in_lod_box(biome_realm realm, const ChunkAlignedBox& box)
{
    std::int32_t min_y = 0;
    std::int32_t max_y = 0;

    switch(realm) {
        case BIOME_REALM_SURFACE:
            min_y = constant::SURFACE_MIN_CHUNK_Y;
            max_y = constant::SURFACE_MAX_CHUNK_Y;
            break;

        case BIOME_REALM_UNDERGROUND:
            min_y = constant::UNDERGROUND_MIN_CHUNK_Y;
            max_y = constant::UNDERGROUND_MAX_CHUNK_Y;
            break;

        case BIOME_REALM_THE_DEPTHS:
            min_y = constant::THE_DEPTHS_MIN_CHUNK_Y;
            max_y = constant::THE_DEPTHS_MAX_CHUNK_Y;
            break;

        case BIOME_REALM_SKY:
            min_y = constant::SKY_MIN_CHUNK_Y;
            max_y = constant::SKY_MAX_CHUNK_Y;
            break;

        default:
            return false;
    }

    return box.min().y() <= max_y && box.max().y() >= min_y;
}

static std::int32_t near_lod_end(const ChunkAlignedBox& view, const ChunkAlignedBox& lod, const ChunkPos& player)
{
    auto full_radius = view_radius(view, player);
    auto width = std::max<std::int32_t>(1, full_radius / 4);
    return std::min(lod_radius(lod, player), full_radius + width);
}

static bool box_contains_xz(const ChunkAlignedBox& box, const ChunkPos& pos)
{
    auto inside = true;
    inside = inside && pos.x() >= box.min().x() && pos.x() <= box.max().x();
    inside = inside && pos.z() >= box.min().z() && pos.z() <= box.max().z();
    return inside;
}

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

static Eigen::Vector2f tile_center_offset(const LodKey& key, const ChunkPos& player)
{
    auto edge = static_cast<float>(lod::tile_edge_chunks(key.level));

    Eigen::Vector2f offset;
    offset.x() = static_cast<float>(key.x) * edge + edge * 0.5f - static_cast<float>(player.x());
    offset.y() = static_cast<float>(key.z) * edge + edge * 0.5f - static_cast<float>(player.z());
    return offset;
}

static std::int32_t axis_distance_min(std::int32_t value, std::int32_t min, std::int32_t max)
{
    if(value < min) {
        return min - value;
    }

    if(value > max) {
        return value - max;
    }

    return 0;
}

static std::int32_t axis_distance_max(std::int32_t value, std::int32_t min, std::int32_t max)
{
    return std::max(iabs(value - min), iabs(value - max));
}

static void rebuild(Session& session, const ChunkPos& player)
{
    session.stream_wanted_chunks.clear();
    session.stream_wanted_lod_chunks.clear();
    session.stream_wanted_lod_tiles.clear();
    session.stream_cursor_chunks = 0;
    session.stream_cursor_lod_chunks = 0;
    session.stream_cursor_lod_tiles = 0;
    session.stream_rebuild = false;

    auto view = session.stream_view_box;
    auto lod = session.stream_lod_box;

    for(auto it = session.stream_sent_chunks.begin(); it != session.stream_sent_chunks.end();) {
        if(view.contains(*it)) {
            it = std::next(it);
        }
        else {
            it = session.stream_sent_chunks.erase(it);
        }
    }

    if(box_is_valid(view)) {
        for(auto x = view.min().x(); x <= view.max().x(); ++x) {
            for(auto y = view.min().y(); y <= view.max().y(); ++y) {
                for(auto z = view.min().z(); z <= view.max().z(); ++z) {
                    auto cpos = ChunkPos(x, y, z);

                    if(!session.stream_sent_chunks.contains(cpos)) {
                        session.stream_wanted_chunks.push_back(cpos);
                    }
                }
            }
        }

        std::sort(session.stream_wanted_chunks.begin(), session.stream_wanted_chunks.end(), [player](const auto& a, const auto& b) {
            auto delta_a = (a - player).template cast<float>();
            auto delta_b = (b - player).template cast<float>();
            return delta_a.squaredNorm() < delta_b.squaredNorm();
        });
    }

    auto realm = utils::realm(player.y());
    auto full_radius = box_is_valid(view) ? view_radius(view, player) : 0;
    auto outer_radius = box_is_valid(lod) ? lod_radius(lod, player) : 0;
    auto near_end = box_is_valid(lod) ? near_lod_end(view, lod, player) : 0;
    auto ring_start = std::max<std::int32_t>(0, full_radius - LOD_RING_OVERLAP);

    if(realm != BIOME_REALM_VOID && box_is_valid(view) && box_is_valid(lod) && near_end > full_radius) {
        for(auto cx = player.x() - near_end; cx <= player.x() + near_end; cx += 1) {
            for(auto cz = player.z() - near_end; cz <= player.z() + near_end; cz += 1) {
                auto distance = std::max(iabs(cx - player.x()), iabs(cz - player.z()));

                if(distance <= ring_start || distance >= near_end) {
                    continue;
                }

                if(cx < lod.min().x() || cx > lod.max().x() || cz < lod.min().z() || cz > lod.max().z()) {
                    continue;
                }

                for(auto cy = player.y() - LOD_Y_RADIUS; cy <= player.y() + LOD_Y_RADIUS; cy += 1) {
                    auto cpos = ChunkPos(cx, cy, cz);

                    if(!session.stream_sent_lod_chunks.contains(cpos)) {
                        session.stream_wanted_lod_chunks.push_back(cpos);
                    }
                }
            }
        }

        std::sort(session.stream_wanted_lod_chunks.begin(), session.stream_wanted_lod_chunks.end(), [player](const auto& a, const auto& b) {
            auto delta_a = (a - player).template cast<float>();
            auto delta_b = (b - player).template cast<float>();
            return delta_a.squaredNorm() < delta_b.squaredNorm();
        });
    }

    for(auto it = session.stream_sent_lod_chunks.begin(); it != session.stream_sent_lod_chunks.end();) {
        auto pos = *it;
        auto distance = std::max(iabs(pos.x() - player.x()), iabs(pos.z() - player.z()));
        auto keep = box_contains_xz(lod, pos);
        keep = keep && iabs(pos.y() - player.y()) <= LOD_Y_RADIUS;
        keep = keep && distance > ring_start && distance < near_end;

        if(keep) {
            it = std::next(it);
        }
        else {
            it = session.stream_sent_lod_chunks.erase(it);
        }
    }

    if(realm != BIOME_REALM_VOID && box_is_valid(lod)) {
        // Level 0 is a one-block column heightmap and starts at the player,
        // so it can fill holes inside the normal view. Every next level
        // doubles the cell and its ring is measured in view distances.
        auto level_min = 0;
        auto level_max = floor_log2(std::max(outer_radius / std::max(full_radius, 1), 1));
        level_max = std::min<std::int32_t>(level_max, LOD_TILE_LEVEL_MAX);
        level_min = std::min(level_min, level_max);

        constexpr std::array<biome_realm, 2> domains = {
            BIOME_REALM_SURFACE,
            BIOME_REALM_SKY,
        };

        for(auto tile_realm : domains) {
            if(!realm_in_lod_box(tile_realm, lod)) {
                continue;
            }

            for(auto level = level_min; level <= level_max; level += 1) {
                auto edge = 1 << (level + 1);
                auto inner = (level == 0) ? 0 : full_radius * (1 << level);
                auto outer = full_radius * (1 << (level + 1));

                auto tile_min_x = floor_div(lod.min().x(), edge);
                auto tile_max_x = floor_div(lod.max().x(), edge);
                auto tile_min_z = floor_div(lod.min().z(), edge);
                auto tile_max_z = floor_div(lod.max().z(), edge);

                auto search_min_x = floor_div(player.x() - outer - edge, edge);
                auto search_max_x = floor_div(player.x() + outer + edge, edge);
                auto search_min_z = floor_div(player.z() - outer - edge, edge);
                auto search_max_z = floor_div(player.z() + outer + edge, edge);

                tile_min_x = std::max(tile_min_x, search_min_x);
                tile_max_x = std::min(tile_max_x, search_max_x);
                tile_min_z = std::max(tile_min_z, search_min_z);
                tile_max_z = std::min(tile_max_z, search_max_z);

                for(auto tx = tile_min_x; tx <= tile_max_x; tx += 1) {
                    for(auto tz = tile_min_z; tz <= tile_max_z; tz += 1) {
                        auto min_x = tx * edge;
                        auto max_x = min_x + edge - 1;
                        auto min_z = tz * edge;
                        auto max_z = min_z + edge - 1;

                        auto min_distance = std::max(axis_distance_min(player.x(), min_x, max_x),
                            axis_distance_min(player.z(), min_z, max_z));
                        auto max_distance = std::max(axis_distance_max(player.x(), min_x, max_x),
                            axis_distance_max(player.z(), min_z, max_z));

                        if(min_distance >= outer || max_distance < inner) {
                            continue;
                        }

                        LodKey key {};
                        key.realm = tile_realm;
                        key.level = static_cast<std::uint8_t>(level);
                        key.x = tx;
                        key.z = tz;

                        if(!session.stream_sent_lod_tiles.contains(key)) {
                            session.stream_wanted_lod_tiles.push_back(key);
                        }
                    }
                }
            }
        }

        std::sort(session.stream_wanted_lod_tiles.begin(), session.stream_wanted_lod_tiles.end(),
            [player](const LodKey& a, const LodKey& b) {
                return tile_center_offset(a, player).squaredNorm() < tile_center_offset(b, player).squaredNorm();
            });
    }

    for(auto it = session.stream_sent_lod_tiles.begin(); it != session.stream_sent_lod_tiles.end();) {
        if(tile_intersects_box(*it, lod)) {
            it = std::next(it);
        }
        else {
            it = session.stream_sent_lod_tiles.erase(it);
        }
    }
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    for(auto& session : sessions::all()) {
        if(session.stream_sent_chunks.erase(event.pos())) {
            session.stream_rebuild = true;
        }
    }
}

static void on_chunk_sent(const ChunkSentEvent& event)
{
    auto session = sessions::find(event.peer());

    if(session == nullptr) {
        return;
    }

    if(event.kind() == chunk_want::FULL) {
        session->stream_sent_chunks.insert(event.pos());
    }
    else {
        session->stream_sent_lod_chunks.insert(event.pos());
    }
}

static void on_chunk_lod_updated(const ChunkLodUpdatedEvent& event)
{
    // Every session that already holds the old pyramid gets the new one, so
    // an edited chunk never keeps drawing its old geometry out there.
    for(auto& session : sessions::all()) {
        if(session.peer == nullptr) {
            continue;
        }

        if(session.stream_sent_lod_chunks.erase(event.pos())) {
            receive::enqueue_chunk(session.peer, event.pos(), chunk_want::LOD);
        }
    }
}

void streaming::init(void)
{
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<ChunkSentEvent>().connect<&on_chunk_sent>();
    globals::dispatcher.sink<ChunkLodUpdatedEvent>().connect<&on_chunk_lod_updated>();
}

void streaming::shutdown(void)
{
    // The world is purged after the sessions are gone, and a purge fires a
    // removal event per chunk. Stop listening before that can reach the planner
    globals::dispatcher.sink<ChunkRemoveEvent>().disconnect<&on_chunk_remove>();
    globals::dispatcher.sink<ChunkSentEvent>().disconnect<&on_chunk_sent>();
    globals::dispatcher.sink<ChunkLodUpdatedEvent>().disconnect<&on_chunk_lod_updated>();
}

void streaming::fixed_update_late(void)
{
    ZoneScoped;

    for(auto& session : sessions::all()) {
        if(session.peer == nullptr) {
            continue;
        }

        if(!globals::registry.valid(session.player) || !globals::registry.all_of<Transform>(session.player)) {
            continue;
        }

        const auto& transform = globals::registry.get<Transform>(session.player);

        if(session.stream_rebuild) {
            rebuild(session, transform.chunk);
        }

        while(session.stream_cursor_chunks < session.stream_wanted_chunks.size() && session.stream_budget_chunks > 0) {
            auto cpos = session.stream_wanted_chunks[session.stream_cursor_chunks];
            session.stream_cursor_chunks += 1;

            if(session.stream_sent_chunks.contains(cpos)) {
                continue;
            }

            receive::enqueue_chunk(session.peer, cpos, chunk_want::FULL);
            session.stream_budget_chunks -= 1;
        }

        while(session.stream_cursor_lod_chunks < session.stream_wanted_lod_chunks.size() && session.stream_budget_lod_chunks > 0) {
            auto cpos = session.stream_wanted_lod_chunks[session.stream_cursor_lod_chunks];
            session.stream_cursor_lod_chunks += 1;

            if(session.stream_sent_lod_chunks.contains(cpos)) {
                continue;
            }

            receive::enqueue_chunk(session.peer, cpos, chunk_want::LOD);
            session.stream_budget_lod_chunks -= 1;
        }

        while(session.stream_cursor_lod_tiles < session.stream_wanted_lod_tiles.size() && session.stream_budget_lod_tiles > 0) {
            auto key = session.stream_wanted_lod_tiles[session.stream_cursor_lod_tiles];

            if(session.stream_sent_lod_tiles.contains(key)) {
                session.stream_cursor_lod_tiles += 1;
                continue;
            }

            if(!lod_tile::request(key, session.peer)) {
                break;
            }

            session.stream_sent_lod_tiles.insert(key);
            session.stream_cursor_lod_tiles += 1;
            session.stream_budget_lod_tiles -= 1;
        }
    }
}

void streaming::subscribe(Session& session, const packet::World_Request& packet)
{
    if(!globals::registry.valid(session.player) || !globals::registry.all_of<Transform>(session.player)) {
        return;
    }

    const auto& transform = globals::registry.get<Transform>(session.player);
    const auto& player = transform.chunk;

    auto allowed_view = utils::view_box(player, static_cast<ChunkPos::value_type>(interest::view_distance.value()));
    auto view = intersect_boxes(packet.view, allowed_view);

    auto lod = make_empty_box();

    if((packet.flags & packet::World_Request::FLAG_LOD) && server_vars::lod_distance.value() > 0) {
        auto allowed_lod = utils::view_box(player, static_cast<ChunkPos::value_type>(server_vars::lod_distance.value()));
        lod = intersect_boxes(packet.lod, allowed_lod);
    }

    auto changed = !box_is_same(session.stream_view_box, view);
    changed = changed || !box_is_same(session.stream_lod_box, lod);
    changed = changed || (session.stream_player_chunk.array() != player.array()).any();

    session.stream_view_box = view;
    session.stream_lod_box = lod;
    session.stream_player_chunk = player;
    session.stream_budget_chunks = std::min(packet.chunk_budget, MAX_CHUNK_BUDGET);
    session.stream_budget_lod_chunks = std::min(packet.lod_chunk_budget, MAX_LOD_CHUNK_BUDGET);
    session.stream_budget_lod_tiles = std::min(packet.lod_tile_budget, MAX_LOD_TILE_BUDGET);

    if(changed) {
        session.stream_rebuild = true;
    }
}
