#include "server/pch.hh"

#include "server/net/streaming.hh"

#include "shared/component/transform.hh"
#include "shared/constant.hh"
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

static void rebuild(Session& session, const ChunkPos& player)
{
    session.stream_wanted_chunks.clear();
    session.stream_cursor_chunks = 0;
    session.stream_rebuild = false;

    auto view = session.stream_view_box;

    auto it = session.stream_sent_chunks.begin();

    while(it != session.stream_sent_chunks.end()) {
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
        if(session.world.get() == event.world()) {
            if(session.stream_sent_chunks.erase(event.pos())) {
                session.stream_rebuild = true;
            }
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

void streaming::subscribe(Session* session, const packet::World_Request& packet)
{
    if(session->world == nullptr) {
        return;
    }

    auto& registry = session->world->registry();

    if(!registry.valid(session->player) || !registry.all_of<Transform>(session->player)) {
        return;
    }

    const auto& transform = registry.get<Transform>(session->player);
    auto& player = transform.chunk;

    auto radius = static_cast<ChunkPos::value_type>(interest::view_distance.value());
    auto allowed = utils::view_box(player, radius);
    auto view_min = ChunkPos(packet.view.min().cwiseMax(allowed.min()));
    auto view_max = ChunkPos(packet.view.max().cwiseMin(allowed.max()));

    // A chunk the world does not hold can never
    // arrive, so the box is clipped to its height
    auto& definition = session->world->definition();
    auto min_y = static_cast<ChunkPos::value_type>(definition.min_y >> constant::CHUNK_SIZE_LOG2);
    auto max_y = static_cast<ChunkPos::value_type>(definition.max_y >> constant::CHUNK_SIZE_LOG2);

    view_min.y() = std::max(view_min.y(), min_y);
    view_max.y() = std::min(view_max.y(), max_y);

    auto view = ChunkAlignedBox(view_min, view_max);

    auto allowed_interest = utils::interest_box(player, radius);
    auto interest_min = ChunkPosXZ(packet.interest.min().cwiseMax(allowed_interest.min()));
    auto interest_max = ChunkPosXZ(packet.interest.max().cwiseMin(allowed_interest.max()));
    auto column_interest = ChunkAlignedBoxXZ(interest_min, interest_max);

    auto changed = false;
    changed = changed || !session->stream_view_box.isApprox(view, 0);
    changed = changed || (session->stream_player_chunk.array() != player.array()).any();

    // A column the client dropped has to travel
    // again when the player comes back; biomes stay
    if(!session->interest_box.isApprox(column_interest, 0)) {
        auto it = session->stream_sent_columns.begin();

        while(it != session->stream_sent_columns.end()) {
            if(column_interest.contains(*it)) {
                it = std::next(it);
            }
            else {
                it = session->stream_sent_columns.erase(it);
            }
        }
    }

    session->stream_view_box = view;
    session->interest_box = column_interest;
    session->stream_player_chunk = player;
    session->stream_budget_chunks = std::min(packet.chunk_budget, MAX_CHUNK_BUDGET);

    if(changed) {
        session->stream_rebuild = true;
    }
}
