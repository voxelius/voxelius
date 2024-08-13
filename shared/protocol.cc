// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/math/floathacks.hh>
#include <shared/globals.hh>
#include <shared/protocol.hh>
#include <shared/rwbuffer.hh>
#include <vector>

static RWBuffer read_buffer = {};
static RWBuffer write_buffer = {};

void protocol::send_packet(ENetPeer *peer, const protocol::StatusRequest &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI32(write_buffer, packet.version);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::StatusResponse &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI32(write_buffer, packet.version);
    RWBuffer::write_UI16(write_buffer, packet.max_players);
    RWBuffer::write_UI16(write_buffer, packet.num_players);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::LoginRequest &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI32(write_buffer, packet.version);
    RWBuffer::write_UI64(write_buffer, packet.password_hash);
    RWBuffer::write_UI64(write_buffer, packet.vdef_checksum);
    RWBuffer::write_UI64(write_buffer, packet.player_uid);
    RWBuffer::write_string(write_buffer, packet.username);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::LoginResponse &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI16(write_buffer, packet.session_id);
    RWBuffer::write_UI16(write_buffer, packet.tickrate);
    RWBuffer::write_string(write_buffer, packet.username);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::Disconnect &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_string(write_buffer, packet.reason);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::CreateEntity &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    RWBuffer::write_UI32(write_buffer, packet.type);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::ChunkVoxels &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    RWBuffer::write_I32(write_buffer, packet.chunk[0]);
    RWBuffer::write_I32(write_buffer, packet.chunk[1]);
    RWBuffer::write_I32(write_buffer, packet.chunk[2]);
    for(std::size_t i = 0; i < CHUNK_VOLUME; RWBuffer::write_UI16(write_buffer, packet.voxels[i++]));
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityTransform &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    RWBuffer::write_I32(write_buffer, packet.coord.chunk[0]);
    RWBuffer::write_I32(write_buffer, packet.coord.chunk[1]);
    RWBuffer::write_I32(write_buffer, packet.coord.chunk[2]);
    RWBuffer::write_FP32(write_buffer, packet.coord.local[0]);
    RWBuffer::write_FP32(write_buffer, packet.coord.local[1]);
    RWBuffer::write_FP32(write_buffer, packet.coord.local[2]);
    RWBuffer::write_FP32(write_buffer, packet.angles[0]);
    RWBuffer::write_FP32(write_buffer, packet.angles[1]);
    RWBuffer::write_FP32(write_buffer, packet.angles[2]);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityHead &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    RWBuffer::write_FP32(write_buffer, packet.angles[0]);
    RWBuffer::write_FP32(write_buffer, packet.angles[1]);
    RWBuffer::write_FP32(write_buffer, packet.angles[2]);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityVelocity &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    RWBuffer::write_FP32(write_buffer, packet.value[0]);
    RWBuffer::write_FP32(write_buffer, packet.value[1]);
    RWBuffer::write_FP32(write_buffer, packet.value[2]);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::SpawnPlayer &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));    
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::ChatMessage &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_UI16(write_buffer, packet.type);
    RWBuffer::write_string(write_buffer, packet.message);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::SetVoxel &packet)
{
    RWBuffer::setup(write_buffer);
    RWBuffer::write_I64(write_buffer, packet.coord[0]);
    RWBuffer::write_I64(write_buffer, packet.coord[1]);
    RWBuffer::write_I64(write_buffer, packet.coord[2]);
    RWBuffer::write_UI16(write_buffer, packet.voxel);
    RWBuffer::write_UI16(write_buffer, packet.flags);
    enet_peer_send(peer, 0, RWBuffer::submit(write_buffer, ENET_PACKET_FLAG_RELIABLE));
}

void protocol::handle_packet(const ENetPacket *packet, ENetPeer *peer)
{
    RWBuffer::setup(read_buffer, packet->data, packet->dataLength);

    protocol::StatusRequest status_request = {};
    protocol::StatusResponse status_response = {};
    protocol::LoginRequest login_request = {};
    protocol::LoginResponse login_response = {};
    protocol::Disconnect disconnect = {};
    protocol::CreateEntity create_entity = {};
    protocol::ChunkVoxels chunk_voxels = {};
    protocol::EntityTransform entity_transform = {};
    protocol::EntityHead entity_head = {};
    protocol::EntityVelocity entity_velocity = {};
    protocol::SpawnPlayer spawn_player = {};
    protocol::ChatMessage chat_message = {};
    protocol::SetVoxel set_voxel = {};
    
    switch(RWBuffer::read_UI16(read_buffer)) {
        case protocol::StatusRequest::ID:
            status_request.peer = peer;
            status_request.version = RWBuffer::read_UI32(read_buffer);
            globals::dispatcher.trigger(status_request);
            break;
        case protocol::StatusResponse::ID:
            status_response.peer = peer;
            status_response.version = RWBuffer::read_UI32(read_buffer);
            status_response.max_players = RWBuffer::read_UI16(read_buffer);
            status_response.num_players = RWBuffer::read_UI16(read_buffer);
            globals::dispatcher.trigger(status_response);
            break;
        case protocol::LoginRequest::ID:
            login_request.peer = peer;
            login_request.version = RWBuffer::read_UI32(read_buffer);
            login_request.password_hash = RWBuffer::read_UI64(read_buffer);
            login_request.vdef_checksum = RWBuffer::read_UI64(read_buffer);
            login_request.player_uid = RWBuffer::read_UI64(read_buffer);
            login_request.username = RWBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(login_request);
            break;
        case protocol::LoginResponse::ID:
            login_response.peer = peer;
            login_response.session_id = RWBuffer::read_UI16(read_buffer);
            login_response.tickrate = RWBuffer::read_UI16(read_buffer);
            login_response.username = RWBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(login_response);
            break;
        case protocol::Disconnect::ID:
            disconnect.peer = peer;
            disconnect.reason = RWBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(disconnect);
            break;
        case protocol::CreateEntity::ID:
            create_entity.peer = peer;
            create_entity.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            create_entity.type = RWBuffer::read_UI32(read_buffer);
            globals::dispatcher.trigger(create_entity);
            break;
        case protocol::ChunkVoxels::ID:
            chunk_voxels.peer = peer;
            chunk_voxels.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            chunk_voxels.chunk[0] = RWBuffer::read_I32(read_buffer);
            chunk_voxels.chunk[1] = RWBuffer::read_I32(read_buffer);
            chunk_voxels.chunk[2] = RWBuffer::read_I32(read_buffer);
            for(std::size_t i = 0; i < CHUNK_VOLUME; chunk_voxels.voxels[i++] = RWBuffer::read_UI16(read_buffer));
            globals::dispatcher.trigger(chunk_voxels);
            break;
        case protocol::EntityTransform::ID:
            entity_transform.peer = peer;
            entity_transform.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            entity_transform.coord.chunk[0] = RWBuffer::read_I32(read_buffer);
            entity_transform.coord.chunk[1] = RWBuffer::read_I32(read_buffer);
            entity_transform.coord.chunk[2] = RWBuffer::read_I32(read_buffer);
            entity_transform.coord.local[0] = RWBuffer::read_FP32(read_buffer);
            entity_transform.coord.local[1] = RWBuffer::read_FP32(read_buffer);
            entity_transform.coord.local[2] = RWBuffer::read_FP32(read_buffer);
            entity_transform.angles[0] = RWBuffer::read_FP32(read_buffer);
            entity_transform.angles[1] = RWBuffer::read_FP32(read_buffer);
            entity_transform.angles[2] = RWBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_transform);
            break;
        case protocol::EntityHead::ID:
            entity_head.peer = peer;
            entity_head.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            entity_head.angles[0] = RWBuffer::read_FP32(read_buffer);
            entity_head.angles[1] = RWBuffer::read_FP32(read_buffer);
            entity_head.angles[2] = RWBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_head);
            break;
        case protocol::EntityVelocity::ID:
            entity_velocity.peer = peer;
            entity_velocity.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            entity_velocity.value[0] = RWBuffer::read_FP32(read_buffer);
            entity_velocity.value[1] = RWBuffer::read_FP32(read_buffer);
            entity_velocity.value[2] = RWBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_velocity);
            break;
        case protocol::SpawnPlayer::ID:
            spawn_player.peer = peer;
            spawn_player.entity = static_cast<entt::entity>(RWBuffer::read_UI64(read_buffer));
            globals::dispatcher.trigger(spawn_player);
            break;
        case protocol::ChatMessage::ID:
            chat_message.peer = peer;
            chat_message.type = RWBuffer::read_UI16(read_buffer);
            chat_message.message = RWBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(chat_message);
            break;
        case protocol::SetVoxel::ID:
            set_voxel.peer = peer;
            set_voxel.coord[0] = RWBuffer::read_I64(read_buffer);
            set_voxel.coord[1] = RWBuffer::read_I64(read_buffer);
            set_voxel.coord[2] = RWBuffer::read_I64(read_buffer);
            set_voxel.voxel = RWBuffer::read_UI16(read_buffer);
            set_voxel.flags = RWBuffer::read_UI16(read_buffer);
            globals::dispatcher.trigger(set_voxel);
            break;
    }
}
