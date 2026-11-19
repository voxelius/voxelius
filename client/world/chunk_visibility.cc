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

constexpr static std::size_t REQUESTS_PER_TICK = 128;
constexpr static std::size_t REQUEST_BACKLOG_LIMIT = 2048;
constexpr static std::uint16_t MAX_CHUNK_BUDGET = 256;

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

    packet::World_Request packet;
    packet.view = s_bounds_curr;
    packet.chunk_budget = static_cast<std::uint16_t>(budget);
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
}
