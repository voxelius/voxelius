#include "server/pch.hh"

#include "server/net/receive.hh"

#include "shared/component/transform.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/globals.hh"
#include "shared/net/packet_player.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/physics/physics.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/chunk.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/sessions.hh"
#include "server/net/streaming.hh"
#include "server/system/pmove_validator.hh"
#include "server/world/chunk_loader.hh"

struct PendingChunkSend final {
    ENetPeer* peer;
    ChunkPos cpos;
};

struct ChunkRequest final {
    ENetPeer* peer;
};

static vx::hash_map<ChunkPos, std::vector<ChunkRequest>> s_waiting_peers;
static vx::hash_set<ChunkPos> s_sent_chunks;
static std::vector<packet::World_Light::Entry> s_pending_light;

constexpr static std::size_t CHUNK_SEND_BUDGET = 256;

// A batch is capped so a tick that relights half the view distance does not
// turn into one giant datagram.
constexpr static std::size_t LIGHT_BATCH_BYTES = 32768;

static std::deque<PendingChunkSend> s_send_queue;
static vx::hash_map<ENetPeer*, vx::hash_set<ChunkPos>> s_send_queued;

static void enqueue_send(ENetPeer* peer, const ChunkPos& cpos)
{
    auto& queued = s_send_queued[peer];

    if(queued.insert(cpos).second) {
        s_send_queue.push_back(PendingChunkSend { peer, cpos });
    }
}

static void enqueue_peer(const ChunkPos& cpos, ENetPeer* peer)
{
    auto& waiting = s_waiting_peers[cpos];

    auto known = std::find_if(waiting.begin(), waiting.end(), [peer](const auto& request) {
        return request.peer == peer;
    });

    if(known == waiting.end()) {
        waiting.push_back(ChunkRequest { peer });
    }
}

static void send_chunk_now(ENetPeer* peer, const ChunkPos& cpos)
{
    auto chunk = globals::world->find_chunk(cpos);

    if(chunk == nullptr) {
        // The chunk was unloaded between queueing and sending. Ask for it
        // again instead of dropping the peer's request on the floor.
        enqueue_peer(cpos, peer);
        chunk_loader::request(cpos);
        return;
    }

    auto realm = utils::realm(cpos.y());

    if(realm != BIOME_REALM_VOID) {
        packet::World_Biomes biomes_packet;
        biomes_packet.realm = realm;
        biomes_packet.cpos = ChunkPosXZ(cpos.x(), cpos.z());
        biomes_packet.biomes = globals::world->biomes(realm, biomes_packet.cpos);
        protocol::send(biomes_packet, peer);
    }

    packet::World_Blocks blocks_packet;
    blocks_packet.cpos = cpos;
    blocks_packet.blocks = chunk->blocks();
    blocks_packet.light = chunk->light();
    blocks_packet.light_valid = chunk->has_light();
    protocol::send(blocks_packet, peer);

    s_sent_chunks.insert(cpos);

    globals::dispatcher.trigger(ChunkSentEvent(peer, cpos));
}

static void send_waiting(const ChunkPos& cpos)
{
    auto it = s_waiting_peers.find(cpos);

    if(it == s_waiting_peers.end()) {
        return;
    }

    auto chunk = globals::world->find_chunk(cpos);

    if(chunk == nullptr) {
        // Still loading/generating; retry on the next sweep.
        return;
    }

    for(const auto& request : it->second) {
        enqueue_send(request.peer, cpos);
    }

    s_waiting_peers.erase(it);
}

static void on_world_request(const packet::World_Request& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    streaming::subscribe(*session, packet);
}

static void on_chunk_ready(const ChunkReadyEvent& event)
{
    send_waiting(event.pos());
}

static void on_chunk_light(const ChunkLightUpdateEvent& event)
{
    // Drain the change set even when nothing is sent: a stale entry must not
    // carry over into the next tick.
    auto change = lighting::take_change(event.pos());

    if(0 == s_sent_chunks.count(event.pos())) {
        // No client knows this chunk yet; it will be sent with fresh
        // light in World_Blocks.
        return;
    }

    auto chunk = globals::world->find_chunk(event.pos());

    if(chunk == nullptr || !chunk->has_light()) {
        return;
    }

    packet::World_Light::Entry entry;

    entry.cpos = event.pos();

    if(change.full) {
        entry.full = true;
        entry.light = chunk->light();
    }
    else {
        if(change.indices.empty()) {
            return;
        }

        entry.full = false;
        entry.indices = std::move(change.indices);
        entry.values.reserve(entry.indices.size());

        auto& light = chunk->light();

        for(auto index : entry.indices) {
            entry.values.push_back(light.get_packed(index));
        }
    }

    s_pending_light.push_back(std::move(entry));
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    s_sent_chunks.erase(event.pos());
}

static void on_player_attack_e(const packet::Player_AttackE& packet)
{
    // empty
}

static bool in_reach(entt::entity player, const BlockPos& bpos)
{
    auto& transform = globals::registry.get<Transform>(player);
    auto player_bpos = utils::to_block(transform.chunk, transform.local.cast<LocalPos::value_type>());
    auto distance = (bpos - player_bpos).cast<float>().norm();
    return distance <= gamerules::reach_distance.value();
}

// The client predicts block changes locally. A rejected action must send
// the server state back for every position it could have touched, or the
// prediction stays in the client world until it is rebuilt from scratch.
static void send_correction(ENetPeer* peer, const BlockPos& bpos)
{
    auto cpos = utils::to_chunk(bpos);
    auto chunk = globals::world->find_chunk(cpos);

    if(chunk == nullptr) {
        // The client kept a chunk the server no longer has. Resending the
        // whole chunk rolls back every prediction made against it.
        enqueue_peer(cpos, peer);
        chunk_loader::request(cpos);
        return;
    }

    packet::World_SetBlock correction {};
    correction.bpos = bpos;
    correction.block = chunk->get_block(utils::to_local(bpos));
    protocol::send(correction, peer);
}

static BlockPos placed_position(const packet::Player_InteractB& packet)
{
    return packet.bpos + packet.normal.cast<BlockPos::value_type>();
}

static void reject_break(ENetPeer* peer, const BlockPos& bpos)
{
    send_correction(peer, bpos);
}

static void reject_place(ENetPeer* peer, const packet::Player_InteractB& packet)
{
    send_correction(peer, packet.bpos);
    send_correction(peer, placed_position(packet));
}

static void on_player_attack_b(const packet::Player_AttackB& packet)
{
    if(!gamerules::allow_building.value()) {
        return;
    }

    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    if(!in_reach(session->player, packet.bpos)) {
        reject_break(packet.peer, packet.bpos);
        return;
    }

    if(globals::world->get_block(packet.bpos) != packet.expected) {
        reject_break(packet.peer, packet.bpos);
        return;
    }

    physics::BlockHit hit {};
    hit.world = globals::world.get();
    hit.block_pos = packet.bpos;
    hit.id = packet.expected;

    utils::block_break(hit, session->player);
}

static void on_player_interact_e(const packet::Player_InteractE& packet)
{
    // empty
}

static void on_player_interact_b(const packet::Player_InteractB& packet)
{
    if(!gamerules::allow_building.value()) {
        return;
    }

    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    if(!in_reach(session->player, packet.bpos)) {
        reject_place(packet.peer, packet);
        return;
    }

    if(globals::world->get_block(packet.bpos) != packet.expected) {
        reject_place(packet.peer, packet);
        return;
    }

    physics::BlockHit hit {};
    hit.world = globals::world.get();
    hit.block_pos = packet.bpos;
    hit.id = packet.expected;
    hit.face = packet.face;
    hit.normal = packet.normal;
    hit.point = packet.point;

    // TODO: hardcoded placed block until an inventory/hotbar system exists.
    // Keep it in sync with client/system/player_target.cc.
    auto family = block_registry::find_family(Identifier::from_string("builtin:emitter"));
    auto block = family ? family->default_variant : block_registry::find(Identifier::from_string("builtin:emitter"));

    utils::block_place(hit, session->player, block);
}

static void on_player_move_data(const packet::Player_MoveData& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    globals::registry.emplace_or_replace<MoveData>(session->player, MoveData { packet.wishdir });

    SimulatedMoveData sim_data;
    sim_data.chunk = packet.simulated_cpos;
    sim_data.local = packet.simulated_lpos;
    sim_data.velocity = packet.velocity;

    globals::registry.emplace_or_replace<SimulatedMoveData>(session->player, std::move(sim_data));
}

// ENet transmits a reliable packet only while its window has room, and that
// window shrinks when the round trip grows. Handing it more than the window
// holds does not send faster: it builds a queue, the queue delay inflates the
// round trip, the throttle drops, the window shrinks further and throughput
// collapses. So a chunk waits here until the window can take it.
static bool peer_window_full(const ENetPeer* peer)
{
    auto window = (peer->packetThrottle * peer->windowSize) / ENET_PEER_PACKET_THROTTLE_SCALE;

    return peer->reliableDataInTransit >= std::max(window, static_cast<enet_uint32>(peer->mtu));
}

static void process_send_queue(void)
{
    static std::deque<PendingChunkSend> deferred;

    std::size_t sent = 0;

    deferred.clear();

    while(!s_send_queue.empty() && sent < CHUNK_SEND_BUDGET) {
        auto request = s_send_queue.front();

        s_send_queue.pop_front();

        if(request.peer == nullptr) {
            continue;
        }

        if(peer_window_full(request.peer)) {
            // Keep it for the next tick without blocking the peers that can
            // still take data.
            deferred.push_back(std::move(request));
            continue;
        }

        auto queued = s_send_queued.find(request.peer);

        if(queued != s_send_queued.cend()) {
            queued->second.erase(request.cpos);

            if(queued->second.empty()) {
                s_send_queued.erase(queued);
            }
        }

        send_chunk_now(request.peer, request.cpos);
        sent += 1;
    }

    s_send_queue.insert(s_send_queue.end(), deferred.begin(), deferred.end());
}

static std::size_t light_entry_size(const packet::World_Light::Entry& entry)
{
    constexpr auto HEADER = 3 * sizeof(std::int32_t) + sizeof(std::uint8_t);

    if(entry.full) {
        auto& light = entry.light;
        auto light_data = light.data();
        return HEADER + light_data.size();
    }

    constexpr auto CELL = sizeof(std::uint16_t) + sizeof(std::uint8_t);

    return HEADER + sizeof(std::uint16_t) + entry.indices.size() * CELL;
}

// One packet per peer per tick instead of one per changed chunk. The fill can
// touch dozens of chunks in a tick, and each of those used to be its own small
// datagram.
static void flush_light_updates(void)
{
    if(s_pending_light.empty()) {
        return;
    }

    for(auto& session : sessions::all()) {
        if(session.peer == nullptr) {
            continue;
        }

        packet::World_Light packet;
        std::size_t size = 0;

        for(const auto& entry : s_pending_light) {
            if(!session.view_box.contains(entry.cpos)) {
                continue;
            }

            auto entry_size = light_entry_size(entry);

            if(size > 0 && size + entry_size > LIGHT_BATCH_BYTES) {
                protocol::send(packet, session.peer);
                packet.entries.clear();
                size = 0;
            }

            size += entry_size;
            packet.entries.push_back(entry);
        }

        if(!packet.entries.empty()) {
            protocol::send(packet, session.peer);
        }
    }

    s_pending_light.clear();
}

void receive::fixed_update_late(void)
{
    ZoneScoped;

    if(!s_waiting_peers.empty()) {
        std::vector<ChunkPos> keys;
        keys.reserve(s_waiting_peers.size());

        for(const auto& it : s_waiting_peers) {
            keys.push_back(it.first);
        }

        for(const auto& cpos : keys) {
            send_waiting(cpos);
        }
    }

    // Light goes out before the budgeted chunk batch, so it is not stuck
    // behind the world load.
    flush_light_updates();
    process_send_queue();
}

void receive::forget_peer(ENetPeer* peer)
{
    std::erase_if(s_send_queue, [peer](const PendingChunkSend& request) {
        return request.peer == peer;
    });

    s_send_queued.erase(peer);

    std::vector<ChunkPos> empty;

    for(auto& it : s_waiting_peers) {
        auto& requests = it.second;

        std::erase_if(requests, [peer](const auto& request) {
            return request.peer == peer;
        });

        if(requests.empty()) {
            empty.push_back(it.first);
        }
    }

    for(const auto& cpos : empty) {
        s_waiting_peers.erase(cpos);
    }
}

void receive::enqueue_chunk(ENetPeer* peer, const ChunkPos& cpos)
{
    if(globals::world->find_chunk(cpos)) {
        enqueue_send(peer, cpos);
        return;
    }

    enqueue_peer(cpos, peer);
    chunk_loader::request(cpos);
}

void receive::init(void)
{
    globals::dispatcher.sink<packet::World_Request>().connect<&on_world_request>();

    globals::dispatcher.sink<packet::Player_AttackE>().connect<&on_player_attack_e>();
    globals::dispatcher.sink<packet::Player_AttackB>().connect<&on_player_attack_b>();
    globals::dispatcher.sink<packet::Player_InteractE>().connect<&on_player_interact_e>();
    globals::dispatcher.sink<packet::Player_InteractB>().connect<&on_player_interact_b>();
    globals::dispatcher.sink<packet::Player_MoveData>().connect<&on_player_move_data>();

    globals::dispatcher.sink<ChunkReadyEvent>().connect<&on_chunk_ready>();
    globals::dispatcher.sink<ChunkLightUpdateEvent>().connect<&on_chunk_light>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
}

ChunkSentEvent::ChunkSentEvent(ENetPeer* peer, const ChunkPos& pos) : m_peer(peer), m_pos(pos)
{
    // empty
}
