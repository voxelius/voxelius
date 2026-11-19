#include "client/pch.hh"

#include "client/world/chunk_visibility.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/view.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/net/session.hh"
#include "client/world/chunk_mesher.hh"
#include "client/world/lod_chunk_mesher.hh"
#include "client/world/lod_manager.hh"

constexpr static std::size_t REQUESTS_PER_TICK = 128;
constexpr static std::size_t REQUEST_BACKLOG_LIMIT = 2048;
constexpr static std::uint16_t MAX_CHUNK_BUDGET = 256;
constexpr static std::uint16_t LOD_CHUNK_BUDGET = 16;
constexpr static std::uint16_t LOD_TILE_BUDGET = 16;

static ChunkAlignedBox s_bounds_curr;

static ChunkAlignedBox empty_box(void)
{
    return ChunkAlignedBox(ChunkPos::Ones(), ChunkPos::Zero());
}

void chunk_visibility::fixed_update_late(void)
{
    ZoneScoped;

    static std::vector<ChunkPos> to_remove;

    if(session::state != SESSION_INGAME) {
        s_bounds_curr = empty_box();
        lod_manager::evict_outside(s_bounds_curr);
        return;
    }

    auto radius = static_cast<ChunkPos::value_type>(camera::view_distance.value());
    s_bounds_curr = utils::view_box(camera::chunk, radius);

    auto backlog = chunk_mesher::backlog();
    std::size_t budget = 0;

    if(backlog < REQUEST_BACKLOG_LIMIT) {
        budget = REQUESTS_PER_TICK * (REQUEST_BACKLOG_LIMIT - backlog) / REQUEST_BACKLOG_LIMIT;
        budget = std::max<std::size_t>(budget, 1);
    }

    budget = std::min<std::size_t>(budget, MAX_CHUNK_BUDGET);

    auto lod_radius = static_cast<ChunkPos::value_type>(lod_manager::range_chunks());
    auto lod_box = lod_manager::enabled() ? utils::view_box(camera::chunk, lod_radius) : empty_box();

    // The LOD stream follows the same backpressure as the chunk stream: a
    // client that cannot mesh what it already has must not order more.
    auto lod_backlog = lod_chunk_mesher::backlog();
    auto lod_budget = std::uint16_t { 0 };

    if(lod_backlog < REQUEST_BACKLOG_LIMIT) {
        auto scaled = static_cast<std::size_t>(LOD_CHUNK_BUDGET) * (REQUEST_BACKLOG_LIMIT - lod_backlog) / REQUEST_BACKLOG_LIMIT;
        lod_budget = static_cast<std::uint16_t>(std::max<std::size_t>(scaled, 1));
    }

    packet::World_Request packet;
    packet.view = s_bounds_curr;
    packet.lod = lod_box;
    packet.flags = lod_manager::enabled() ? packet::World_Request::FLAG_LOD : 0;
    packet.chunk_budget = static_cast<std::uint16_t>(budget);
    packet.lod_chunk_budget = lod_budget;
    packet.lod_tile_budget = LOD_TILE_BUDGET;
    protocol::send(packet, globals::peer);

    auto view = world::chunk_registry.view<Chunk_Component>();

    to_remove.clear();

    for(const auto [entity, component] : view.each()) {
        if(s_bounds_curr.contains(component.position)) {
            continue;
        }

        to_remove.push_back(component.position);
    }

    for(const auto& cpos : to_remove) {
        world::remove_chunk(cpos);
    }

    lod_manager::evict_outside(lod_box);
}
