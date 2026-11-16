#include "shared/pch.hh"

#include "shared/net/protocol.hh"

#include "core/buffer.hh"

#include "shared/net/packet_auth.hh"
#include "shared/net/packet_cvar.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/packet_msg.hh"
#include "shared/net/packet_player.hh"
#include "shared/net/packet_session.hh"
#include "shared/net/packet_status.hh"
#include "shared/net/packet_world.hh"

static void common_broadcast(ENetPacket* packet, ENetHost* host, ENetPeer* except)
{
    assert(host);

    if(packet) {
        if(except) {
            for(unsigned int i = 0; i < host->peerCount; ++i) {
                if(host->peers[i].state == ENET_PEER_STATE_CONNECTED) {
                    if(&host->peers[i] == except) {
                        continue;
                    }

                    enet_peer_send(&host->peers[i], protocol::CHANNEL, packet);
                }
            }
        }
        else {
            enet_host_broadcast(host, protocol::CHANNEL, packet);
        }
    }
}

static void common_send(ENetPacket* packet, ENetPeer* peer)
{
    assert(peer);

    if(packet) {
        enet_peer_send(peer, protocol::CHANNEL, packet);
    }
}

void protocol::decode(entt::dispatcher& dispatcher, const ENetPacket* packet, ENetPeer* peer)
{
    static ReadBuffer buffer;

    static packet::Status_Request status_request;
    static packet::Status_Response status_response;
    static packet::Auth_Request auth_request;
    static packet::Auth_Challenge auth_challenge;
    static packet::Auth_Response auth_response;
    static packet::Auth_Admission auth_admission;
    static packet::Session_Disconnect session_disconnect;
    static packet::World_Request world_request_chunk;
    static packet::World_Blocks world_chunk_blocks;
    static packet::World_Biomes world_chunk_biomes;
    static packet::World_Light world_chunk_light;
    static packet::World_SetBlock world_set_block;
    static packet::World_Timings world_timings;
    static packet::Cvar_ServerVars cvar_server_vars;
    static packet::Cvar_ServerCmds cvar_server_cmds;
    static packet::Cvar_Command cvar_command;
    static packet::Msg_Log msg_log;
    static packet::Player_AttackE player_attack_e;
    static packet::Player_AttackB player_attack_b;
    static packet::Player_InteractE player_interact_e;
    static packet::Player_InteractB player_interact_b;
    static packet::Player_MoveData player_move_data;
    static packet::Entity_Spawn entity_spawn;
    static packet::Entity_Patch entity_patch;
    static packet::Entity_Remove entity_remove;
    static packet::Entity_Client entity_client;

    assert(packet);
    assert(peer);

    buffer.reset(packet);

    auto id = buffer.read<std::uint16_t>();
    auto type = static_cast<packet::packet_type>(id);

    switch(type) {
        case packet::Status_Request::TYPE:
            packet::Status_Request::decode(status_request, buffer);
            status_request.peer = peer;
            dispatcher.trigger(static_cast<const packet::Status_Request&>(status_request));
            break;

        case packet::Status_Response::TYPE:
            packet::Status_Response::decode(status_response, buffer);
            status_response.peer = peer;
            dispatcher.trigger(static_cast<const packet::Status_Response&>(status_response));
            break;

        case packet::Auth_Request::TYPE:
            packet::Auth_Request::decode(auth_request, buffer);
            auth_request.peer = peer;
            dispatcher.trigger(static_cast<const packet::Auth_Request&>(auth_request));
            break;

        case packet::Auth_Challenge::TYPE:
            packet::Auth_Challenge::decode(auth_challenge, buffer);
            auth_challenge.peer = peer;
            dispatcher.trigger(static_cast<const packet::Auth_Challenge&>(auth_challenge));
            break;

        case packet::Auth_Response::TYPE:
            packet::Auth_Response::decode(auth_response, buffer);
            auth_response.peer = peer;
            dispatcher.trigger(static_cast<const packet::Auth_Response&>(auth_response));
            break;

        case packet::Auth_Admission::TYPE:
            packet::Auth_Admission::decode(auth_admission, buffer);
            auth_admission.peer = peer;
            dispatcher.trigger(static_cast<const packet::Auth_Admission&>(auth_admission));
            break;

        case packet::Session_Disconnect::TYPE:
            packet::Session_Disconnect::decode(session_disconnect, buffer);
            session_disconnect.peer = peer;
            dispatcher.trigger(static_cast<const packet::Session_Disconnect&>(session_disconnect));
            break;

        case packet::World_Request::TYPE:
            packet::World_Request::decode(world_request_chunk, buffer);
            world_request_chunk.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_Request&>(world_request_chunk));
            break;

        case packet::World_Blocks::TYPE:
            packet::World_Blocks::decode(world_chunk_blocks, buffer);
            world_chunk_blocks.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_Blocks&>(world_chunk_blocks));
            break;

        case packet::World_Biomes::TYPE:
            packet::World_Biomes::decode(world_chunk_biomes, buffer);
            world_chunk_biomes.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_Biomes&>(world_chunk_biomes));
            break;

        case packet::World_Light::TYPE:
            packet::World_Light::decode(world_chunk_light, buffer);
            world_chunk_light.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_Light&>(world_chunk_light));
            break;

        case packet::World_SetBlock::TYPE:
            packet::World_SetBlock::decode(world_set_block, buffer);
            world_set_block.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_SetBlock&>(world_set_block));
            break;

        case packet::World_Timings::TYPE:
            packet::World_Timings::decode(world_timings, buffer);
            world_timings.peer = peer;
            dispatcher.trigger(static_cast<const packet::World_Timings&>(world_timings));
            break;

        case packet::Cvar_ServerVars::TYPE:
            packet::Cvar_ServerVars::decode(cvar_server_vars, buffer);
            cvar_server_vars.peer = peer;
            dispatcher.trigger(static_cast<const packet::Cvar_ServerVars&>(cvar_server_vars));
            break;

        case packet::Cvar_ServerCmds::TYPE:
            packet::Cvar_ServerCmds::decode(cvar_server_cmds, buffer);
            cvar_server_cmds.peer = peer;
            dispatcher.trigger(static_cast<const packet::Cvar_ServerCmds&>(cvar_server_cmds));
            break;

        case packet::Cvar_Command::TYPE:
            packet::Cvar_Command::decode(cvar_command, buffer);
            cvar_command.peer = peer;
            dispatcher.trigger(static_cast<const packet::Cvar_Command&>(cvar_command));
            break;

        case packet::Msg_Log::TYPE:
            packet::Msg_Log::decode(msg_log, buffer);
            msg_log.peer = peer;
            dispatcher.trigger(static_cast<const packet::Msg_Log&>(msg_log));
            break;

        case packet::Player_AttackE::TYPE:
            packet::Player_AttackE::decode(player_attack_e, buffer);
            player_attack_e.peer = peer;
            dispatcher.trigger(static_cast<const packet::Player_AttackE&>(player_attack_e));
            break;

        case packet::Player_AttackB::TYPE:
            packet::Player_AttackB::decode(player_attack_b, buffer);
            player_attack_b.peer = peer;
            dispatcher.trigger(static_cast<const packet::Player_AttackB&>(player_attack_b));
            break;

        case packet::Player_InteractE::TYPE:
            packet::Player_InteractE::decode(player_interact_e, buffer);
            player_interact_e.peer = peer;
            dispatcher.trigger(static_cast<const packet::Player_InteractE&>(player_interact_e));
            break;

        case packet::Player_InteractB::TYPE:
            packet::Player_InteractB::decode(player_interact_b, buffer);
            player_interact_b.peer = peer;
            dispatcher.trigger(static_cast<const packet::Player_InteractB&>(player_interact_b));
            break;

        case packet::Player_MoveData::TYPE:
            packet::Player_MoveData::decode(player_move_data, buffer);
            player_move_data.peer = peer;
            dispatcher.trigger(static_cast<const packet::Player_MoveData&>(player_move_data));
            break;

        case packet::Entity_Spawn::TYPE:
            packet::Entity_Spawn::decode(entity_spawn, buffer);
            entity_spawn.peer = peer;
            dispatcher.trigger(static_cast<const packet::Entity_Spawn&>(entity_spawn));
            break;

        case packet::Entity_Patch::TYPE:
            packet::Entity_Patch::decode(entity_patch, buffer);
            entity_patch.peer = peer;
            dispatcher.trigger(static_cast<const packet::Entity_Patch&>(entity_patch));
            break;

        case packet::Entity_Remove::TYPE:
            packet::Entity_Remove::decode(entity_remove, buffer);
            entity_remove.peer = peer;
            dispatcher.trigger(static_cast<const packet::Entity_Remove&>(entity_remove));
            break;

        case packet::Entity_Client::TYPE:
            packet::Entity_Client::decode(entity_client, buffer);
            entity_client.peer = peer;
            dispatcher.trigger(static_cast<const packet::Entity_Client&>(entity_client));
            break;
    }
}

template<typename T>
void protocol::broadcast(const T& packet, ENetHost* host, ENetPeer* except)
{
    static WriteBuffer buffer;

    assert(host);

    buffer.reset();
    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(T::TYPE));
    T::encode(packet, buffer);

    auto enet_packet = buffer.to_packet(ENET_PACKET_FLAG_RELIABLE);
    common_broadcast(enet_packet, host, except);
}

template void protocol::broadcast<packet::Status_Request>(const packet::Status_Request& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Status_Response>(const packet::Status_Response& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Auth_Request>(const packet::Auth_Request& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Auth_Challenge>(const packet::Auth_Challenge& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Auth_Response>(const packet::Auth_Response& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Auth_Admission>(const packet::Auth_Admission& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Session_Disconnect>(const packet::Session_Disconnect& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_Request>(const packet::World_Request& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_Blocks>(const packet::World_Blocks& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_Biomes>(const packet::World_Biomes& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_Light>(const packet::World_Light& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_SetBlock>(const packet::World_SetBlock& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::World_Timings>(const packet::World_Timings& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Cvar_ServerVars>(const packet::Cvar_ServerVars& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Cvar_ServerCmds>(const packet::Cvar_ServerCmds& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Cvar_Command>(const packet::Cvar_Command& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Msg_Log>(const packet::Msg_Log& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Player_AttackE>(const packet::Player_AttackE& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Player_AttackB>(const packet::Player_AttackB& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Player_InteractE>(const packet::Player_InteractE& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Player_InteractB>(const packet::Player_InteractB& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Entity_Spawn>(const packet::Entity_Spawn& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Entity_Patch>(const packet::Entity_Patch& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Entity_Remove>(const packet::Entity_Remove& packet, ENetHost* host, ENetPeer* except);
template void protocol::broadcast<packet::Entity_Client>(const packet::Entity_Client& packet, ENetHost* host, ENetPeer* except);

template<typename T>
void protocol::send(const T& packet, ENetPeer* peer)
{
    static WriteBuffer buffer;

    assert(peer);

    buffer.reset();
    buffer.write<std::uint16_t>(static_cast<std::uint16_t>(T::TYPE));
    T::encode(packet, buffer);

    auto enet_packet = buffer.to_packet(ENET_PACKET_FLAG_RELIABLE);
    common_send(enet_packet, peer);
}

template void protocol::send<packet::Status_Request>(const packet::Status_Request& packet, ENetPeer* peer);
template void protocol::send<packet::Status_Response>(const packet::Status_Response& packet, ENetPeer* peer);
template void protocol::send<packet::Auth_Request>(const packet::Auth_Request& packet, ENetPeer* peer);
template void protocol::send<packet::Auth_Challenge>(const packet::Auth_Challenge& packet, ENetPeer* peer);
template void protocol::send<packet::Auth_Response>(const packet::Auth_Response& packet, ENetPeer* peer);
template void protocol::send<packet::Auth_Admission>(const packet::Auth_Admission& packet, ENetPeer* peer);
template void protocol::send<packet::Session_Disconnect>(const packet::Session_Disconnect& packet, ENetPeer* peer);
template void protocol::send<packet::World_Request>(const packet::World_Request& packet, ENetPeer* peer);
template void protocol::send<packet::World_Blocks>(const packet::World_Blocks& packet, ENetPeer* peer);
template void protocol::send<packet::World_Biomes>(const packet::World_Biomes& packet, ENetPeer* peer);
template void protocol::send<packet::World_Light>(const packet::World_Light& packet, ENetPeer* peer);
template void protocol::send<packet::World_SetBlock>(const packet::World_SetBlock& packet, ENetPeer* peer);
template void protocol::send<packet::World_Timings>(const packet::World_Timings& packet, ENetPeer* peer);
template void protocol::send<packet::Cvar_ServerVars>(const packet::Cvar_ServerVars& packet, ENetPeer* peer);
template void protocol::send<packet::Cvar_ServerCmds>(const packet::Cvar_ServerCmds& packet, ENetPeer* peer);
template void protocol::send<packet::Cvar_Command>(const packet::Cvar_Command& packet, ENetPeer* peer);
template void protocol::send<packet::Msg_Log>(const packet::Msg_Log& packet, ENetPeer* peer);
template void protocol::send<packet::Player_AttackE>(const packet::Player_AttackE& packet, ENetPeer* peer);
template void protocol::send<packet::Player_AttackB>(const packet::Player_AttackB& packet, ENetPeer* peer);
template void protocol::send<packet::Player_InteractE>(const packet::Player_InteractE& packet, ENetPeer* peer);
template void protocol::send<packet::Player_InteractB>(const packet::Player_InteractB& packet, ENetPeer* peer);
template void protocol::send<packet::Player_MoveData>(const packet::Player_MoveData& packet, ENetPeer* peer);
template void protocol::send<packet::Entity_Spawn>(const packet::Entity_Spawn& packet, ENetPeer* peer);
template void protocol::send<packet::Entity_Patch>(const packet::Entity_Patch& packet, ENetPeer* peer);
template void protocol::send<packet::Entity_Remove>(const packet::Entity_Remove& packet, ENetPeer* peer);
template void protocol::send<packet::Entity_Client>(const packet::Entity_Client& packet, ENetPeer* peer);
