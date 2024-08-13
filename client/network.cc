// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hh>
#include <client/globals.hh>
#include <client/network.hh>
#include <shared/protocol/packets/add_entity.hh>
#include <shared/protocol/packets/chunk_voxels.hh>
#include <shared/protocol/packets/disconnect.hh>
#include <shared/protocol/packets/entity_angles.hh>
#include <shared/protocol/packets/entity_angvel.hh>
#include <shared/protocol/packets/entity_head.hh>
#include <shared/protocol/packets/entity_position.hh>
#include <shared/protocol/packets/entity_transform.hh>
#include <shared/protocol/packets/entity_velocity.hh>
#include <shared/protocol/packets/login_start.hh>
#include <shared/protocol/packets/login_success.hh>
#include <shared/protocol/packets/remove_chunk.hh>
#include <shared/protocol/packets/remove_entity.hh>
#include <shared/protocol/packets/server_info.hh>
#include <shared/protocol/packets/spawn_player.hh>

constexpr static std::uint64_t TICK_DT = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(protocol::TICKRATE));

static std::uint64_t next_update = 0;

static void handle_packet(ENetPeer *peer, const ENetPacket *packet)
{
    PacketReader reader = {};
    PacketReader::setup(reader, packet);

    protocol::AddEntity add_entity = {};
    protocol::ChunkVoxels chunk_voxels = {};
    protocol::Disconnect disconnect = {};
    protocol::EntityAngles entity_angles = {};
    protocol::EntityAngVel entity_angvel = {};
    protocol::EntityHead entity_head = {};
    protocol::EntityPosition entity_position = {};
    protocol::EntityTransform entity_transform = {};
    protocol::EntityVelocity entity_velocity = {};
    protocol::LoginSuccess login_success = {};
    protocol::RemoveChunk remove_chunk = {};
    protocol::RemoveEntity remove_entity = {};
    protocol::ServerInfo server_info = {};
    protocol::SpawnPlayer spawn_player = {};

    switch(PacketReader::UI16(reader)) {
        case protocol::AddEntity::ID:
            add_entity.peer = event.peer;
            protocol::decode_packet(reader, add_entity);
            globals::dispatcher.trigger(add_entity);
            break;
        case protocol::ChunkVoxels::ID:
            chunk_voxels.peer = event.peer;
            protocol::decode_packet(reader, chunk_voxels);
            globals::dispatcher.trigger(chunk_voxels);
            break;
        case protocol::Disconnect::ID:
            disconnect.peer = event.peer;
            protocol::decode_packet(reader, disconnect);
            globals::dispatcher.trigger(disconnect);
            break;
        case protocol::EntityAngles::ID:
            entity_angles.peer = event.peer;
            protocol::decode_packet(reader, entity_angles);
            globals::dispatcher.trigger(entity_angles);
            break;
        case protocol::EntityAngVel::ID:
            entity_angvel.peer = event.peer;
            protocol::decode_packet(reader, entity_angvel);
            globals::dispatcher.trigger(entity_angvel);
            break;
        case protocol::EntityHead::ID:
            entity_head.peer = event.peer;
            protocol::decode_packet(reader, entity_head);
            globals::dispatcher.trigger(entity_head);
            break;
        case protocol::EntityPosition::ID:
            entity_position.peer = event.peer;
            protocol::decode_packet(reader, entity_position);
            globals::dispatcher.trigger(entity_position);
            break;
        case protocol::EntityTransform::ID:
            entity_transform.peer = event.peer;
            protocol::decode_packet(reader, entity_transform);
            globals::dispatcher.trigger(entity_transform);
            break;
        case protocol::EntityVelocity::ID:
            entity_velocity.peer = event.peer;
            protocol::decode_packet(reader, entity_velocity);
            globals::dispatcher.trigger(entity_velocity);
            break;
        case protocol::LoginSuccess::ID:
            login_success.peer = event.peer;
            protocol::decode_packet(reader, login_success);
            globals::dispatcher.trigger(login_success);
            break;
        case protocol::RemoveChunk::ID:
            remove_chunk.peer = event.peer;
            protocol::decode_packet(reader, remove_chunk);
            globals::dispatcher.trigger(remove_chunk);
            break;
        case protocol::RemoveEntity::ID:
            remove_entity.peer = event.peer;
            protocol::decode_packet(reader, remove_entity);
            globals::dispatcher.trigger(remove_entity);
            break;
        case protocol::ServerInfo::ID:
            server_info.peer = event.peer;
            protocol::decode_packet(reader, server_info);
            globals::dispatcher.trigger(server_info);
            break;
        case protocol::SpawnPlayer::ID:
            spawn_player.peer = event.peer;
            protocol::decode_packet(reader, spawn_player);
            globals::dispatcher.trigger(spawn_player);
            break;
    }
}

void client_network::init(void)
{
    globals::host = enet_host_create(nullptr, 1, 2, 0, 0);
    
    if(!globals::host) {
        spdlog::critical("network: unable to create an ENet host");
        std::terminate();
    }
    
    globals:peer = nullptr;
    globals::state = ClientState::Disconnected;
    globals::player = entt::null;
    globals::session_id = UINT16_MAX;
}

void client_network::deinit(void)
{
    client_network::disconnect("Client shutting down");
    enet_host_destroy(globals::host);
}

void client_network::update(void)
{
    if(globals::curtime >= next_update) {
        next_update = globals::curtime + TICK_DT;
        
        ENetEvent event = {};

        while(enet_host_service(globals::host, &event, 0) > 0) {
            if(event.type == ENET_EVENT_TYPE_CONNECT) {
                globals::peer = event.peer;
                globals::state = ClientState::Login;
                
                protocol::Handshake handshake = {};
                handshake.version = protocol::VERSION;
                handshake.intention = protocol::Handshake::LOGIN;
                
                PacketWriter writer = {};
                PacketWriter::UI16(writer, protocol::Handshake::ID);
                protocol::encode_packet(writer, handshake);
                
                enet_peer_send(event.peer, 0, PacketWriter::submit(writer, ENET_PACKET_FLAG_RELIABLE));
                
                protocol::LoginStart login_start = {};
                login_start.player_uid = 42; // FIXME
                login_start.password_crc = 42; // FIXME
                login_start.vdef_checksum = 42; // FIXME
                login_start.username = "dingus"; // FIXME
                
                PacketWriter::setup(writer);
                PacketWriter::UI16(writer, protocol::LoginStart::ID);
                protocol::encode_packet(writer, login_start);
                
                enet_peer_send(event.peer, 0, PacketWriter::submit(writer, ENET_PACKET_FLAG_RELIABLE));
                
                continue;
            }
            
            if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                handle_packet(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                continue;
            }

            if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                enet_peer_reset(event.peer);
                
                globals:peer = nullptr;
                globals::state = ClientState::Disconnected;
                globals::player = entt::null;
                globals::session_id = UINT16_MAX;

                continue;
            }
        }
    }
}

void client_network::connect(const std::string &host, std::uint16_t port)
{
    ENetAddress address = {};
    enet_addresss_set_host(&address, host.c_str());
    address.port = port;
    
    globals::peer = enet_host_connect(globals::host, &address, 1, 0);
    
    if(globals::peer)
        globals::state = ClientState::Connecting;
    else globals::state = ClientState::Disconnected;
}

void client_network::disconnect(const std::string &reason)
{
    if(globals::peer) {
        protocol::Disconnect packet = {};
        packet.reason = reason;
        
        PacketWriter writer = {};
        PacketWriter::UI16(writer, protocol::Disconnect::ID);
        protocol::encode_packet(writer, packet);
        
        enet_peer_send(globals::peer, 0, PacketWriter::submit(writer, ENET_PACKET_FLAG_RELIABLE));
    }
}
