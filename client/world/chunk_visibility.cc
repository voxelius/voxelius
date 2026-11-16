#include "client/pch.hh"

#include "client/world/chunk_visibility.hh"

#include "core/msg/log.hh"

#include "shared/coord.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/view.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/gui/slider.hh"
#include "client/net/session.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_mesher.hh"

struct ChunkRequest final {
    ChunkPos position;
    unsigned distance;
};

constexpr static std::size_t REQUESTS_PER_TICK = 48;

// Requests that were sent but not answered yet. Bounding them is what keeps
// the client from ordering more chunks than it can take in: without it the
// server hands everything to ENet at once, the packets queue up behind the
// network, and the player flies ahead of a stream that is still in flight.
// With compression on the wire a chunk is cheap, so the window can be wide.
constexpr static std::size_t MAX_OUTSTANDING = 256;

// A request nobody answered in this many ticks is forgotten, so a rejected or
// lost one cannot stall the loader forever.
constexpr static std::uint64_t OUTSTANDING_TIMEOUT = 100;

// Chunks that quietly failed to arrive are asked for again at this interval
// while the loader is otherwise idle, instead of waiting for the player to
// cross a chunk boundary and rebuild the whole list.
constexpr static std::uint64_t REFILL_INTERVAL = 20;

// How far ahead of the player the request order is biased, in chunks. While
// flying, what is about to be reached matters more than what is around.
constexpr static float LOOKAHEAD_CHUNKS = 4.0f;

// A client that asks for chunks faster than it can mesh them only builds a
// queue it will never catch up with. The budget shrinks as the mesh queue
// grows, so the loader settles at the mesh rate instead of stopping dead and
// then bursting.
constexpr static std::size_t REQUEST_BACKLOG_LIMIT = 512;

constexpr static std::uint64_t LOG_INTERVAL = 100;

struct PendingRequest final {
    ChunkPos position;
    std::uint64_t tick;
};

static ChunkAlignedBox s_bounds_curr;
static ChunkAlignedBox s_bounds_prev;
static std::vector<ChunkRequest> s_requests;
static std::deque<PendingRequest> s_outstanding;
static vx::hash_set<ChunkPos> s_outstanding_positions;

static std::size_t s_requests_sent;
static std::size_t s_chunks_removed;
static std::uint64_t s_last_log_tick;
static std::uint64_t s_last_refill_tick;

static bool is_outstanding(const ChunkPos& cpos)
{
    return s_outstanding_positions.contains(cpos);
}

static void build_requests(void)
{
    s_requests.clear();

    const auto& min = s_bounds_curr.min();
    const auto& max = s_bounds_curr.max();

    auto focus = camera::lookahead(LOOKAHEAD_CHUNKS);
    auto origin = camera::chunk.cast<float>() + focus;

    for(auto cx = min.x(); cx <= max.x(); cx += 1) {
        for(auto cy = min.y(); cy <= max.y(); cy += 1) {
            for(auto cz = min.z(); cz <= max.z(); cz += 1) {
                auto cpos = ChunkPos(cx, cy, cz);

                auto want_request = false;
                want_request = want_request || nullptr == world::find_chunk(cpos);
                want_request = want_request || !s_bounds_prev.contains(cpos);

                if(want_request && is_outstanding(cpos)) {
                    // Already on its way; asking again only repeats traffic.
                    want_request = false;
                }

                if(want_request) {
                    ChunkRequest request {};
                    request.position = cpos;
                    request.distance = static_cast<unsigned>((cpos.cast<float>() - origin).squaredNorm());
                    s_requests.emplace_back(std::move(request));
                }
            }
        }
    }

    std::sort(s_requests.begin(), s_requests.end(), [](const auto& a, const auto& b) {
        return a.distance > b.distance;
    });
}

void chunk_visibility::note_received(const ChunkPos& cpos)
{
    auto it = std::find_if(s_outstanding.begin(), s_outstanding.end(), [&cpos](const auto& entry) {
        return entry.position == cpos;
    });

    if(it != s_outstanding.end()) {
        s_outstanding.erase(it);
        s_outstanding_positions.erase(cpos);
    }
}

void chunk_visibility::fixed_update_late(void)
{
    ZoneScoped;

    static std::vector<ChunkPos> to_remove;

    if(session::state == SESSION_INGAME) {
        auto radius = static_cast<ChunkPos::value_type>(camera::view_distance.value());
        s_bounds_curr = utils::view_box(camera::chunk, radius);

        auto tick = globals::current_tick;

        while(!s_outstanding.empty()) {
            auto age = tick > s_outstanding.front().tick ? tick - s_outstanding.front().tick : 0;

            if(age <= OUTSTANDING_TIMEOUT) {
                break;
            }

            s_outstanding_positions.erase(s_outstanding.front().position);
            s_outstanding.pop_front();
        }

        auto refill = s_requests.empty() && s_outstanding.empty() && tick > s_last_refill_tick
            && tick - s_last_refill_tick >= REFILL_INTERVAL;

        if(!s_bounds_curr.isApprox(s_bounds_prev) || refill) {
            build_requests();
            s_last_refill_tick = tick;
        }

        auto backlog = chunk_mesher::backlog();
        std::size_t budget = 0;

        if(backlog < REQUEST_BACKLOG_LIMIT) {
            budget = REQUESTS_PER_TICK * (REQUEST_BACKLOG_LIMIT - backlog) / REQUEST_BACKLOG_LIMIT;

            // Always let at least one through: a stalled loader never recovers.
            budget = std::max<std::size_t>(budget, 1);
        }

        if(s_outstanding.size() < MAX_OUTSTANDING) {
            budget = std::min(budget, MAX_OUTSTANDING - s_outstanding.size());
        }
        else {
            budget = 0;
        }

        for(std::size_t i = 0; s_requests.size() && i < budget; ++i) {
            const auto& request = s_requests.back();

            packet::World_Request packet;
            packet.cpos = request.position;
            protocol::send(packet, globals::peer);

            s_outstanding.push_back(PendingRequest { request.position, tick });
            s_outstanding_positions.insert(request.position);
            s_requests.pop_back();
            s_requests_sent += 1;
        }

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

        s_chunks_removed += to_remove.size();

        // The balance of one interval: what was asked for, what was dropped,
        // and what survived. A client that flies into a hole either asked too
        // little, was answered too little, or threw away what it had.
        if(globals::current_tick - s_last_log_tick >= LOG_INTERVAL) {
            s_last_log_tick = globals::current_tick;

            auto loaded = world::chunk_map.size();
            auto meshed = world::chunk_registry.view<ChunkMesh>().size();

            msg::log_info("chunks: loaded={} meshed={} backlog={} requested={} removed={}", loaded, meshed, chunk_mesher::backlog(),
                s_requests_sent, s_chunks_removed);

            s_requests_sent = 0;
            s_chunks_removed = 0;
        }

        s_bounds_prev = s_bounds_curr;
    }
    else {
        s_bounds_prev = ChunkAlignedBox(ChunkPos::Constant(INT32_MIN), ChunkPos::Constant(INT32_MAX));
        s_bounds_curr = ChunkAlignedBox(ChunkPos::Zero(), ChunkPos::Zero());
        s_requests.clear();
        s_outstanding.clear();
        s_outstanding_positions.clear();
    }
}
