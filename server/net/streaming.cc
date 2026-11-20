#include "server/pch.hh"

#include "server/net/streaming.hh"

#include "shared/component/transform.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/view.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/receive.hh"
#include "server/net/sessions.hh"

constexpr static std::uint16_t MAX_CHUNK_BUDGET = 256;

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

static void rebuild(Session& session, const ChunkPos& player)
{
    session.stream_wanted_chunks.clear();
    session.stream_cursor_chunks = 0;
    session.stream_rebuild = false;

    auto view = session.stream_view_box;

    for(auto it = session.stream_sent_chunks.begin(); it != session.stream_sent_chunks.end();) {
        if(view.contains(*it)) {
            it = std::next(it);
        }
        else {
            it = session.stream_sent_chunks.erase(it);
        }
    }

    if(!box_is_valid(view)) {
        return;
    }

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

    session->stream_sent_chunks.insert(event.pos());
}

void streaming::init(void)
{
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<ChunkSentEvent>().connect<&on_chunk_sent>();
}

void streaming::shutdown(void)
{
    // The world is purged after the sessions are gone, and a purge fires a
    // removal event per chunk. Stop listening before that can reach the planner
    globals::dispatcher.sink<ChunkRemoveEvent>().disconnect<&on_chunk_remove>();
    globals::dispatcher.sink<ChunkSentEvent>().disconnect<&on_chunk_sent>();
}

void streaming::fixed_update_late(void)
{
    ZoneScoped;

    for(auto& session : sessions::all()) {
        if(session.peer == nullptr || session.world == nullptr) {
            continue;
        }

        auto& registry = session.world->registry();

        if(!registry.valid(session.player) || !registry.all_of<Transform>(session.player)) {
            continue;
        }

        const auto& transform = registry.get<Transform>(session.player);

        if(session.stream_rebuild) {
            rebuild(session, transform.chunk);
        }

        while(session.stream_cursor_chunks < session.stream_wanted_chunks.size() && session.stream_budget_chunks > 0) {
            auto cpos = session.stream_wanted_chunks[session.stream_cursor_chunks];
            session.stream_cursor_chunks += 1;

            if(session.stream_sent_chunks.contains(cpos)) {
                continue;
            }

            receive::enqueue_chunk(session.peer, cpos);
            session.stream_budget_chunks -= 1;
        }
    }
}

void streaming::subscribe(Session& session, const packet::World_Request& packet)
{
    if(session.world == nullptr) {
        return;
    }

    auto& registry = session.world->registry();

    if(!registry.valid(session.player) || !registry.all_of<Transform>(session.player)) {
        return;
    }

    const auto& transform = registry.get<Transform>(session.player);
    const auto& player = transform.chunk;

    auto allowed_view = utils::view_box(player, static_cast<ChunkPos::value_type>(interest::view_distance.value()));
    auto view = intersect_boxes(packet.view, allowed_view);

    auto changed = !box_is_same(session.stream_view_box, view);
    changed = changed || (session.stream_player_chunk.array() != player.array()).any();

    session.stream_view_box = view;
    session.stream_player_chunk = player;
    session.stream_budget_chunks = std::min(packet.chunk_budget, MAX_CHUNK_BUDGET);

    if(changed) {
        session.stream_rebuild = true;
    }
}
