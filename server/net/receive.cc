#include "server/pch.hh"

#include "server/net/receive.hh"

#include "core/cvar/host.hh"
#include "core/cvar/number.hh"

#include "shared/component/transform.hh"
#include "shared/cvar/gamerules.hh"
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
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/sessions.hh"
#include "server/system/pmove_validator.hh"
#include "server/world/chunk_loader.hh"

static vx::hash_map<ChunkPos, std::vector<ENetPeer*>> s_waiting_peers;
static cvar::Number<float> s_reach_distance { 8.0f, 0.0f, 32.0f };

static void send_chunk(ENetPeer* peer, const ChunkPos& cpos, const std::shared_ptr<Chunk>& chunk)
{
    auto realm = utils::realm(cpos.y());

    if(realm != BIOME_REALM_VOID) {
        packet::World_Biomes biomes_packet;
        biomes_packet.realm = realm;
        biomes_packet.cpos = ChunkPosXZ(cpos.x(), cpos.z());
        biomes_packet.biomes = biome_map::get(realm, biomes_packet.cpos);
        protocol::send(biomes_packet, peer);
    }

    packet::World_Blocks blocks_packet;
    blocks_packet.cpos = cpos;
    blocks_packet.blocks = chunk->blocks();
    protocol::send(blocks_packet, peer);
}

static void on_world_request(const packet::World_Request& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        return;
    }

    auto& transform = globals::registry.get<Transform>(session->player);
    auto delta = ChunkPos(transform.chunk - packet.cpos);
    auto distance = static_cast<unsigned>(delta.cwiseAbs().maxCoeff());

    if(distance > interest::view_distance.value()) {
        return;
    }

    if(auto chunk = world::find_chunk(packet.cpos)) {
        send_chunk(packet.peer, packet.cpos, chunk);
        return;
    }

    s_waiting_peers[packet.cpos].push_back(packet.peer);

    chunk_loader::request(packet.cpos);
}

static void on_chunk_ready(const ChunkReadyEvent& event)
{
    auto it = s_waiting_peers.find(event.pos());

    if(it == s_waiting_peers.end()) {
        return;
    }

    for(auto peer : it->second) {
        send_chunk(peer, event.pos(), event.chunk());
    }

    s_waiting_peers.erase(it);
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
    return distance <= s_reach_distance.value();
}

// The client predicts block changes locally. A rejected action must send
// the server state back for every position it could have touched, or the
// prediction stays in the client world until it is rebuilt from scratch.
static void send_correction(ENetPeer* peer, const BlockPos& bpos)
{
    auto cpos = utils::to_chunk(bpos);
    auto chunk = world::find_chunk(cpos);

    if(chunk == nullptr) {
        // The client kept a chunk the server no longer has. Resending the
        // whole chunk rolls back every prediction made against it.
        auto& waiting = s_waiting_peers[cpos];

        if(std::find(waiting.begin(), waiting.end(), peer) == waiting.end()) {
            waiting.push_back(peer);
        }

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

    if(world::get_block(packet.bpos) != packet.expected) {
        reject_break(packet.peer, packet.bpos);
        return;
    }

    physics::BlockHit hit {};
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

    if(world::get_block(packet.bpos) != packet.expected) {
        reject_place(packet.peer, packet);
        return;
    }

    physics::BlockHit hit {};
    hit.block_pos = packet.bpos;
    hit.id = packet.expected;
    hit.face = packet.face;
    hit.normal = packet.normal;
    hit.point = packet.point;

    // TODO: hardcoded placed block until an inventory/hotbar system exists
    auto family = block_registry::find_family(Identifier::from_string("builtin:water"));
    auto block = family ? family->default_variant : block_registry::find(Identifier::from_string("builtin:water"));

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

void receive::init(void)
{
    auto& reach_distance_binding = cvar::bind_var(s_reach_distance, "net_reachdistance");
    reach_distance_binding.set_help("Maximum block interaction distance");
    reach_distance_binding.set_flags(FCVAR_ARCHIVE);

    globals::dispatcher.sink<packet::World_Request>().connect<&on_world_request>();

    globals::dispatcher.sink<packet::Player_AttackE>().connect<&on_player_attack_e>();
    globals::dispatcher.sink<packet::Player_AttackB>().connect<&on_player_attack_b>();
    globals::dispatcher.sink<packet::Player_InteractE>().connect<&on_player_interact_e>();
    globals::dispatcher.sink<packet::Player_InteractB>().connect<&on_player_interact_b>();
    globals::dispatcher.sink<packet::Player_MoveData>().connect<&on_player_move_data>();

    globals::dispatcher.sink<ChunkReadyEvent>().connect<&on_chunk_ready>();
}
