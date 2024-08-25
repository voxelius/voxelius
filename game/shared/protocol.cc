// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/message.hh>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/globals.hh>
#include <game/shared/protocol.hh>
#include <mathlib/floathacks.hh>

static Message read_msg = {};
static Message write_msg = {};

void protocol::send_packet(ENetPeer *peer, const protocol::StatusRequest &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::StatusRequest::ID);
    Message::write_UI32(write_msg, packet.version);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::StatusResponse &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::StatusResponse::ID);
    Message::write_UI32(write_msg, packet.version);
    Message::write_UI16(write_msg, packet.max_players);
    Message::write_UI16(write_msg, packet.num_players);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::LoginRequest &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::LoginRequest::ID);
    Message::write_UI32(write_msg, packet.version);
    Message::write_UI64(write_msg, packet.password_hash);
    Message::write_UI64(write_msg, packet.vdef_checksum);
    Message::write_UI64(write_msg, packet.player_uid);
    Message::write_string(write_msg, packet.username);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::LoginResponse &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::LoginResponse::ID);
    Message::write_UI16(write_msg, packet.session_id);
    Message::write_UI16(write_msg, packet.tickrate);
    Message::write_string(write_msg, packet.username);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::Disconnect &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::Disconnect::ID);
    Message::write_string(write_msg, packet.reason);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::ChunkVoxels &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::ChunkVoxels::ID);
    Message::write_UI64(write_msg, static_cast<std::uint64_t>(packet.entity));
    Message::write_I32(write_msg, packet.chunk[0]);
    Message::write_I32(write_msg, packet.chunk[1]);
    Message::write_I32(write_msg, packet.chunk[2]);
    for(std::size_t i = 0; i < CHUNK_VOLUME; Message::write_UI16(write_msg, packet.voxels[i++]));
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityTransform &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::EntityTransform::ID);
    Message::write_UI64(write_msg, static_cast<std::uint64_t>(packet.entity));
    Message::write_I32(write_msg, packet.coord.chunk[0]);
    Message::write_I32(write_msg, packet.coord.chunk[1]);
    Message::write_I32(write_msg, packet.coord.chunk[2]);
    Message::write_FP32(write_msg, packet.coord.local[0]);
    Message::write_FP32(write_msg, packet.coord.local[1]);
    Message::write_FP32(write_msg, packet.coord.local[2]);
    Message::write_FP32(write_msg, packet.angles[0]);
    Message::write_FP32(write_msg, packet.angles[1]);
    Message::write_FP32(write_msg, packet.angles[2]);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityHead &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::EntityHead::ID);
    Message::write_UI64(write_msg, static_cast<std::uint64_t>(packet.entity));
    Message::write_FP32(write_msg, packet.angles[0]);
    Message::write_FP32(write_msg, packet.angles[1]);
    Message::write_FP32(write_msg, packet.angles[2]);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::EntityVelocity &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::EntityVelocity::ID);
    Message::write_UI64(write_msg, static_cast<std::uint64_t>(packet.entity));
    Message::write_FP32(write_msg, packet.angular[0]);
    Message::write_FP32(write_msg, packet.angular[1]);
    Message::write_FP32(write_msg, packet.angular[2]);
    Message::write_FP32(write_msg, packet.linear[0]);
    Message::write_FP32(write_msg, packet.linear[1]);
    Message::write_FP32(write_msg, packet.linear[2]);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::SpawnPlayer &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::SpawnPlayer::ID);
    Message::write_UI64(write_msg, static_cast<std::uint64_t>(packet.entity));    
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::ChatMessage &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::ChatMessage::ID);
    Message::write_UI16(write_msg, packet.type);
    Message::write_string(write_msg, packet.message);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send_packet(ENetPeer *peer, const protocol::SetVoxel &packet)
{
    Message::setup(write_msg);
    Message::write_UI16(write_msg, protocol::SetVoxel::ID);
    Message::write_I64(write_msg, packet.coord[0]);
    Message::write_I64(write_msg, packet.coord[1]);
    Message::write_I64(write_msg, packet.coord[2]);
    Message::write_UI16(write_msg, packet.voxel);
    Message::write_UI16(write_msg, packet.flags);
    enet_peer_send(peer, 0, enet_packet_create(write_msg.vector.data(), write_msg.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::handle_packet(const ENetPacket *packet, ENetPeer *peer)
{
    Message::setup(read_msg, packet->data, packet->dataLength);

    protocol::StatusRequest status_request = {};
    protocol::StatusResponse status_response = {};
    protocol::LoginRequest login_request = {};
    protocol::LoginResponse login_response = {};
    protocol::Disconnect disconnect = {};
    protocol::ChunkVoxels chunk_voxels = {};
    protocol::EntityTransform entity_transform = {};
    protocol::EntityHead entity_head = {};
    protocol::EntityVelocity entity_velocity = {};
    protocol::SpawnPlayer spawn_player = {};
    protocol::ChatMessage chat_message = {};
    protocol::SetVoxel set_voxel = {};
    
    switch(Message::read_UI16(read_msg)) {
        case protocol::StatusRequest::ID:
            status_request.peer = peer;
            status_request.version = Message::read_UI32(read_msg);
            globals::dispatcher.trigger(status_request);
            break;
        case protocol::StatusResponse::ID:
            status_response.peer = peer;
            status_response.version = Message::read_UI32(read_msg);
            status_response.max_players = Message::read_UI16(read_msg);
            status_response.num_players = Message::read_UI16(read_msg);
            globals::dispatcher.trigger(status_response);
            break;
        case protocol::LoginRequest::ID:
            login_request.peer = peer;
            login_request.version = Message::read_UI32(read_msg);
            login_request.password_hash = Message::read_UI64(read_msg);
            login_request.vdef_checksum = Message::read_UI64(read_msg);
            login_request.player_uid = Message::read_UI64(read_msg);
            login_request.username = Message::read_string(read_msg);
            globals::dispatcher.trigger(login_request);
            break;
        case protocol::LoginResponse::ID:
            login_response.peer = peer;
            login_response.session_id = Message::read_UI16(read_msg);
            login_response.tickrate = Message::read_UI16(read_msg);
            login_response.username = Message::read_string(read_msg);
            globals::dispatcher.trigger(login_response);
            break;
        case protocol::Disconnect::ID:
            disconnect.peer = peer;
            disconnect.reason = Message::read_string(read_msg);
            globals::dispatcher.trigger(disconnect);
            break;
        case protocol::ChunkVoxels::ID:
            chunk_voxels.peer = peer;
            chunk_voxels.entity = static_cast<entt::entity>(Message::read_UI64(read_msg));
            chunk_voxels.chunk[0] = Message::read_I32(read_msg);
            chunk_voxels.chunk[1] = Message::read_I32(read_msg);
            chunk_voxels.chunk[2] = Message::read_I32(read_msg);
            for(std::size_t i = 0; i < CHUNK_VOLUME; chunk_voxels.voxels[i++] = Message::read_UI16(read_msg));
            globals::dispatcher.trigger(chunk_voxels);
            break;
        case protocol::EntityTransform::ID:
            entity_transform.peer = peer;
            entity_transform.entity = static_cast<entt::entity>(Message::read_UI64(read_msg));
            entity_transform.coord.chunk[0] = Message::read_I32(read_msg);
            entity_transform.coord.chunk[1] = Message::read_I32(read_msg);
            entity_transform.coord.chunk[2] = Message::read_I32(read_msg);
            entity_transform.coord.local[0] = Message::read_FP32(read_msg);
            entity_transform.coord.local[1] = Message::read_FP32(read_msg);
            entity_transform.coord.local[2] = Message::read_FP32(read_msg);
            entity_transform.angles[0] = Message::read_FP32(read_msg);
            entity_transform.angles[1] = Message::read_FP32(read_msg);
            entity_transform.angles[2] = Message::read_FP32(read_msg);
            globals::dispatcher.trigger(entity_transform);
            break;
        case protocol::EntityHead::ID:
            entity_head.peer = peer;
            entity_head.entity = static_cast<entt::entity>(Message::read_UI64(read_msg));
            entity_head.angles[0] = Message::read_FP32(read_msg);
            entity_head.angles[1] = Message::read_FP32(read_msg);
            entity_head.angles[2] = Message::read_FP32(read_msg);
            globals::dispatcher.trigger(entity_head);
            break;
        case protocol::EntityVelocity::ID:
            entity_velocity.peer = peer;
            entity_velocity.entity = static_cast<entt::entity>(Message::read_UI64(read_msg));
            entity_velocity.angular[0] = Message::read_FP32(read_msg);
            entity_velocity.angular[1] = Message::read_FP32(read_msg);
            entity_velocity.angular[2] = Message::read_FP32(read_msg);
            entity_velocity.linear[0] = Message::read_FP32(read_msg);
            entity_velocity.linear[1] = Message::read_FP32(read_msg);
            entity_velocity.linear[2] = Message::read_FP32(read_msg);
            globals::dispatcher.trigger(entity_velocity);
            break;
        case protocol::SpawnPlayer::ID:
            spawn_player.peer = peer;
            spawn_player.entity = static_cast<entt::entity>(Message::read_UI64(read_msg));
            globals::dispatcher.trigger(spawn_player);
            break;
        case protocol::ChatMessage::ID:
            chat_message.peer = peer;
            chat_message.type = Message::read_UI16(read_msg);
            chat_message.message = Message::read_string(read_msg);
            globals::dispatcher.trigger(chat_message);
            break;
        case protocol::SetVoxel::ID:
            set_voxel.peer = peer;
            set_voxel.coord[0] = Message::read_I64(read_msg);
            set_voxel.coord[1] = Message::read_I64(read_msg);
            set_voxel.coord[2] = Message::read_I64(read_msg);
            set_voxel.voxel = Message::read_UI16(read_msg);
            set_voxel.flags = Message::read_UI16(read_msg);
            globals::dispatcher.trigger(set_voxel);
            break;
    }
}
