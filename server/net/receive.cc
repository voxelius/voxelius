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
#include "server/world/world.hh"

constexpr static std::size_t CHUNK_SEND_BUDGET = 256;

// A batch is capped so a tick that relights half the view distance does not turn into one giant datagram
constexpr static std::size_t LIGHT_BATCH_BYTES = 32768;

struct PendingChunkSend final {
    ENetPeer* peer;
    ChunkPos cpos;
};

struct ChunkRequest final {
    ENetPeer* peer;
};

struct ChunkWaitKey final {
    bool operator==(const ChunkWaitKey& other) const
    {
        return world == other.world && pos == other.pos;
    }

    World* world;
    ChunkPos pos;
};

struct PendingLight final {
    World* world;
    packet::World_Light::Entry entry;
};

template<>
struct std::hash<ChunkWaitKey> final {
    std::size_t operator()(const ChunkWaitKey& key) const
    {
        auto value = std::hash<ChunkPos>()(key.pos);
        value ^= reinterpret_cast<std::size_t>(key.world) * 2654435761;
        return value;
    }
};

static vx::hash_map<ChunkWaitKey, std::vector<ChunkRequest>> s_waiting_peers;
static vx::hash_map<const World*, vx::hash_set<ChunkPos>> s_sent_chunks;
static std::vector<PendingLight> s_pending_light;

static std::deque<PendingChunkSend> s_send_queue;
static vx::hash_map<ENetPeer*, vx::hash_set<ChunkPos>> s_send_queued;

static void enqueue_send(ENetPeer* peer, const ChunkPos& cpos)
{
    auto& queued = s_send_queued[peer];

    if(queued.insert(cpos).second) {
        PendingChunkSend request {};
        request.peer = peer;
        request.cpos = cpos;

        s_send_queue.push_back(request);
    }
}

static void enqueue_peer(World* world, const ChunkPos& cpos, ENetPeer* peer)
{
    assert(world);

    ChunkWaitKey key {};
    key.world = world;
    key.pos = cpos;

    auto& waiting = s_waiting_peers[key];

    auto known = std::find_if(waiting.begin(), waiting.end(), [peer](const auto& request) {
        return request.peer == peer;
    });

    if(known == waiting.end()) {
        ChunkRequest request {};
        request.peer = peer;

        waiting.push_back(request);
    }
}

static void send_chunk_now(ENetPeer* peer, const ChunkPos& cpos)
{
    auto session = sessions::find(peer);

    if(session == nullptr) {
        return;
    }

    auto world = session->world;
    auto chunk = world->find_chunk(cpos);

    if(chunk == nullptr) {
        // The chunk was unloaded between queueing and sending; ask for it again instead of dropping the request
        enqueue_peer(world.get(), cpos, peer);
        chunk_loader::request(world, cpos);
        return;
    }

    auto cpos_xz = ChunkPosXZ(cpos.x(), cpos.z());

    if(session->stream_sent_biomes.insert(cpos_xz).second) {
        packet::World_Biomes biomes_packet;
        biomes_packet.cpos = cpos_xz;
        biomes_packet.biomes = world->biomes(cpos_xz);
        protocol::send(biomes_packet, peer);
    }

    if(session->stream_sent_columns.insert(cpos_xz).second) {
        auto server_world = static_cast<ServerWorld*>(world.get());
        auto& column_chunk = server_world->columns().get(cpos_xz);

        packet::World_Columns columns_packet;
        columns_packet.cpos = cpos_xz;
        columns_packet.columns = column_chunk.columns;
        protocol::send(columns_packet, peer);
    }

    packet::World_Blocks blocks_packet;
    blocks_packet.cpos = cpos;
    blocks_packet.blocks = chunk->blocks();
    blocks_packet.light = chunk->light();
    blocks_packet.light_valid = chunk->has_light();
    protocol::send(blocks_packet, peer);

    s_sent_chunks[world.get()].insert(cpos);

    globals::dispatcher.trigger(ChunkSentEvent(peer, cpos));
}

static void send_waiting(World* world, const ChunkPos& cpos)
{
    assert(world);

    ChunkWaitKey key {};
    key.world = world;
    key.pos = cpos;

    auto it = s_waiting_peers.find(key);

    if(it == s_waiting_peers.end()) {
        return;
    }

    auto chunk = world->find_chunk(cpos);

    if(chunk == nullptr) {
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

    streaming::subscribe(session, packet);
}

static void on_chunk_ready(const ChunkReadyEvent& event)
{
    send_waiting(event.world(), event.pos());
}

static void on_chunk_light(const ChunkLightUpdateEvent& event)
{
    // Drain the change set even when nothing is sent: a stale entry must not carry over into the next tick
    auto world = event.world();
    auto change = lighting::take_change(world, event.pos());

    auto& sent = s_sent_chunks[world];

    if(0 == sent.count(event.pos())) {
        // No client knows this chunk yet; it will be sent with fresh light in World_Blocks
        return;
    }

    auto chunk = world->find_chunk(event.pos());

    if(chunk == nullptr || !chunk->has_light()) {
        return;
    }

    PendingLight pending {};
    pending.world = world;

    auto& entry = pending.entry;

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

    s_pending_light.push_back(std::move(pending));
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    auto it = s_sent_chunks.find(event.world());

    if(it != s_sent_chunks.cend()) {
        it->second.erase(event.pos());
    }
}

static void on_player_attack_e(const packet::Player_AttackE&)
{
    // empty
}

static bool in_reach(entt::registry& registry, entt::entity player, const BlockPos& bpos)
{
    auto& transform = registry.get<Transform>(player);
    auto player_bpos = utils::to_block(transform.chunk, transform.local.cast<LocalPos::value_type>());
    auto delta = (bpos - player_bpos).cast<float>();
    auto distance = delta.norm();
    return distance <= gamerules::reach_distance.value();
}

// The client predicts block changes locally. A rejected action must send the server state back for
// every position it could have touched, or the prediction stays in the client world until it is rebuilt from scratch
static void send_correction(ENetPeer* peer, const BlockPos& bpos)
{
    auto session = sessions::find(peer);

    if(session == nullptr) {
        return;
    }

    auto world = session->world;

    auto cpos = utils::to_chunk(bpos);
    auto chunk = world->find_chunk(cpos);

    if(chunk == nullptr) {
        // The client kept a chunk the server no longer has; resending the whole chunk rolls back every prediction made against it
        enqueue_peer(world.get(), cpos, peer);
        chunk_loader::request(world, cpos);
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

    auto world = session->world;
    auto& registry = world->registry();

    if(!in_reach(registry, session->player, packet.bpos)) {
        reject_break(packet.peer, packet.bpos);
        return;
    }

    if(world->get_block(packet.bpos) != packet.expected) {
        reject_break(packet.peer, packet.bpos);
        return;
    }

    physics::BlockHit hit {};
    hit.world = world.get();
    hit.block_pos = packet.bpos;
    hit.id = packet.expected;

    utils::block_break(hit, session->player);
}

static void on_player_interact_e(const packet::Player_InteractE&)
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

    auto world = session->world;
    auto& registry = world->registry();

    if(!in_reach(registry, session->player, packet.bpos)) {
        reject_place(packet.peer, packet);
        return;
    }

    if(world->get_block(packet.bpos) != packet.expected) {
        reject_place(packet.peer, packet);
        return;
    }

    physics::BlockHit hit {};
    hit.world = world.get();
    hit.block_pos = packet.bpos;
    hit.id = packet.expected;
    hit.face = packet.face;
    hit.normal = packet.normal;
    hit.point = packet.point;

    // TODO: hardcoded placed block until an inventory/hotbar system exists; keep it in sync with client/system/player_target.cc
    auto family = block_registry::find_family(Identifier::from_string("builtin:stone_slab"));
    block_id_type block;

    if(family) {
        block = family->default_variant;
    }
    else {
        block = block_registry::find(Identifier::from_string("builtin:stone_slab"));
    }

    utils::block_place(hit, session->player, block);
}

static Eigen::Vector3f sanitize_direction(Eigen::Vector3f value)
{
    for(Eigen::Index axis = 0; axis < value.size(); axis += 1) {
        auto component = value[axis];

        if(!std::isfinite(component)) {
            component = 0.0f;
        }

        value[axis] = std::clamp(component, -1.0f, 1.0f);
    }

    return value;
}

static Eigen::Vector3f sanitize_vector(Eigen::Vector3f value)
{
    constexpr static float LIMIT = 1.0e6f;

    for(Eigen::Index axis = 0; axis < value.size(); axis += 1) {
        auto component = value[axis];

        if(!std::isfinite(component)) {
            component = 0.0f;
        }

        value[axis] = std::clamp(component, -LIMIT, LIMIT);
    }

    return value;
}

static void on_player_move_data(const packet::Player_MoveData& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    auto& registry = session->world->registry();

    registry.emplace_or_replace<MoveData>(session->player, MoveData { sanitize_direction(packet.wishdir) });

    SimulatedMoveData sim_data;
    sim_data.chunk = packet.simulated_cpos;
    sim_data.local = sanitize_vector(packet.simulated_lpos);
    sim_data.velocity = sanitize_vector(packet.velocity);
    sim_data.angles = sanitize_vector(packet.camera_angles);

    registry.emplace_or_replace<SimulatedMoveData>(session->player, std::move(sim_data));
}

// ENet transmits a reliable packet only while its window has room, and that window shrinks when the round trip grows.
// Handing it more than the window holds does not send faster: it builds a queue, the queue delay inflates the round trip,
// the throttle drops, the window shrinks further and throughput collapses. So a chunk waits here until the window can take it
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
// touch dozens of chunks in a tick, and each of those used to be its own small datagram
static void flush_light_updates(void)
{
    if(s_pending_light.empty()) {
        return;
    }

    for(auto& session : sessions::all()) {
        if(session.peer == nullptr || session.world == nullptr) {
            continue;
        }

        auto world = session.world.get();

        packet::World_Light packet;
        std::size_t size = 0;

        for(const auto& pending : s_pending_light) {
            if(pending.world != world) {
                continue;
            }

            if(!session.view_box.contains(pending.entry.cpos)) {
                continue;
            }

            auto entry_size = light_entry_size(pending.entry);

            if(size > 0 && size + entry_size > LIGHT_BATCH_BYTES) {
                protocol::send(packet, session.peer);
                packet.entries.clear();
                size = 0;
            }

            size += entry_size;
            packet.entries.push_back(pending.entry);
        }

        if(!packet.entries.empty()) {
            protocol::send(packet, session.peer);
        }
    }

    s_pending_light.clear();
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

void receive::fixed_update_late(void)
{
    ZoneScoped;

    if(!s_waiting_peers.empty()) {
        std::vector<ChunkWaitKey> keys;
        keys.reserve(s_waiting_peers.size());

        for(const auto& it : s_waiting_peers) {
            keys.push_back(it.first);
        }

        for(const auto& key : keys) {
            send_waiting(key.world, key.pos);
        }
    }

    // Light goes out before the budgeted chunk batch, so it is not stuck behind the world load
    flush_light_updates();
    process_send_queue();
}

void receive::forget_peer(ENetPeer* peer)
{
    std::erase_if(s_send_queue, [peer](const PendingChunkSend& request) {
        return request.peer == peer;
    });

    s_send_queued.erase(peer);

    std::vector<ChunkWaitKey> empty;

    for(auto& it : s_waiting_peers) {
        auto& requests = it.second;

        std::erase_if(requests, [peer](const auto& request) {
            return request.peer == peer;
        });

        if(requests.empty()) {
            empty.push_back(it.first);
        }
    }

    for(const auto& key : empty) {
        s_waiting_peers.erase(key);
    }
}

void receive::enqueue_chunk(ENetPeer* peer, const ChunkPos& cpos)
{
    auto session = sessions::find(peer);

    if(session == nullptr) {
        return;
    }

    auto world = session->world;

    if(world->find_chunk(cpos)) {
        enqueue_send(peer, cpos);
        return;
    }

    enqueue_peer(world.get(), cpos, peer);
    chunk_loader::request(world, cpos);
}

ChunkSentEvent::ChunkSentEvent(ENetPeer* peer, const ChunkPos& pos) : m_peer(peer), m_pos(pos)
{
    // empty
}
