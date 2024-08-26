// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/packet_buffer.hh>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/globals.hh>
#include <game/shared/protocol.hh>
#include <mathlib/floathacks.hh>

static PacketBuffer read_buffer = {};
static PacketBuffer write_buffer = {};

ENetPacket *protocol::make_packet(const protocol::StatusRequest &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::StatusRequest::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::StatusResponse &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::StatusResponse::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    PacketBuffer::write_UI16(write_buffer, packet.max_players);
    PacketBuffer::write_UI16(write_buffer, packet.num_players);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::LoginRequest &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::LoginRequest::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    PacketBuffer::write_UI64(write_buffer, packet.password_hash);
    PacketBuffer::write_UI64(write_buffer, packet.vdef_checksum);
    PacketBuffer::write_UI64(write_buffer, packet.player_uid);
    PacketBuffer::write_string(write_buffer, packet.username);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::LoginResponse &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::LoginResponse::ID);
    PacketBuffer::write_UI16(write_buffer, packet.session_id);
    PacketBuffer::write_UI16(write_buffer, packet.tickrate);
    PacketBuffer::write_string(write_buffer, packet.username);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::Disconnect &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::Disconnect::ID);
    PacketBuffer::write_string(write_buffer, packet.reason);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::ChunkVoxels &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::ChunkVoxels::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_I32(write_buffer, packet.chunk[0]);
    PacketBuffer::write_I32(write_buffer, packet.chunk[1]);
    PacketBuffer::write_I32(write_buffer, packet.chunk[2]);
    for(std::size_t i = 0; i < CHUNK_VOLUME; PacketBuffer::write_UI16(write_buffer, packet.voxels[i++]));
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::EntityTransform &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::EntityTransform::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_I32(write_buffer, packet.coord.chunk[0]);
    PacketBuffer::write_I32(write_buffer, packet.coord.chunk[1]);
    PacketBuffer::write_I32(write_buffer, packet.coord.chunk[2]);
    PacketBuffer::write_FP32(write_buffer, packet.coord.local[0]);
    PacketBuffer::write_FP32(write_buffer, packet.coord.local[1]);
    PacketBuffer::write_FP32(write_buffer, packet.coord.local[2]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[0]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[1]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[2]);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::EntityHead &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::EntityHead::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_FP32(write_buffer, packet.angles[0]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[1]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[2]);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::EntityVelocity &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::EntityVelocity::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_FP32(write_buffer, packet.angular[0]);
    PacketBuffer::write_FP32(write_buffer, packet.angular[1]);
    PacketBuffer::write_FP32(write_buffer, packet.angular[2]);
    PacketBuffer::write_FP32(write_buffer, packet.linear[0]);
    PacketBuffer::write_FP32(write_buffer, packet.linear[1]);
    PacketBuffer::write_FP32(write_buffer, packet.linear[2]);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::SpawnPlayer &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::SpawnPlayer::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));    
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::ChatMessage &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::ChatMessage::ID);
    PacketBuffer::write_UI16(write_buffer, packet.type);
    PacketBuffer::write_string(write_buffer, packet.message);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

ENetPacket *protocol::make_packet(const protocol::SetVoxel &packet, std::uint32_t flags)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::SetVoxel::ID);
    PacketBuffer::write_I64(write_buffer, packet.coord[0]);
    PacketBuffer::write_I64(write_buffer, packet.coord[1]);
    PacketBuffer::write_I64(write_buffer, packet.coord[2]);
    PacketBuffer::write_UI16(write_buffer, packet.voxel);
    PacketBuffer::write_UI16(write_buffer, packet.flags);
    return enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), flags);
}

void protocol::handle_packet(const ENetPacket *packet, ENetPeer *peer)
{
    PacketBuffer::setup(read_buffer, packet->data, packet->dataLength);

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
    
    switch(PacketBuffer::read_UI16(read_buffer)) {
        case protocol::StatusRequest::ID:
            status_request.peer = peer;
            status_request.version = PacketBuffer::read_UI32(read_buffer);
            globals::dispatcher.trigger(status_request);
            break;
        case protocol::StatusResponse::ID:
            status_response.peer = peer;
            status_response.version = PacketBuffer::read_UI32(read_buffer);
            status_response.max_players = PacketBuffer::read_UI16(read_buffer);
            status_response.num_players = PacketBuffer::read_UI16(read_buffer);
            globals::dispatcher.trigger(status_response);
            break;
        case protocol::LoginRequest::ID:
            login_request.peer = peer;
            login_request.version = PacketBuffer::read_UI32(read_buffer);
            login_request.password_hash = PacketBuffer::read_UI64(read_buffer);
            login_request.vdef_checksum = PacketBuffer::read_UI64(read_buffer);
            login_request.player_uid = PacketBuffer::read_UI64(read_buffer);
            login_request.username = PacketBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(login_request);
            break;
        case protocol::LoginResponse::ID:
            login_response.peer = peer;
            login_response.session_id = PacketBuffer::read_UI16(read_buffer);
            login_response.tickrate = PacketBuffer::read_UI16(read_buffer);
            login_response.username = PacketBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(login_response);
            break;
        case protocol::Disconnect::ID:
            disconnect.peer = peer;
            disconnect.reason = PacketBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(disconnect);
            break;
        case protocol::ChunkVoxels::ID:
            chunk_voxels.peer = peer;
            chunk_voxels.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            chunk_voxels.chunk[0] = PacketBuffer::read_I32(read_buffer);
            chunk_voxels.chunk[1] = PacketBuffer::read_I32(read_buffer);
            chunk_voxels.chunk[2] = PacketBuffer::read_I32(read_buffer);
            for(std::size_t i = 0; i < CHUNK_VOLUME; chunk_voxels.voxels[i++] = PacketBuffer::read_UI16(read_buffer));
            globals::dispatcher.trigger(chunk_voxels);
            break;
        case protocol::EntityTransform::ID:
            entity_transform.peer = peer;
            entity_transform.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            entity_transform.coord.chunk[0] = PacketBuffer::read_I32(read_buffer);
            entity_transform.coord.chunk[1] = PacketBuffer::read_I32(read_buffer);
            entity_transform.coord.chunk[2] = PacketBuffer::read_I32(read_buffer);
            entity_transform.coord.local[0] = PacketBuffer::read_FP32(read_buffer);
            entity_transform.coord.local[1] = PacketBuffer::read_FP32(read_buffer);
            entity_transform.coord.local[2] = PacketBuffer::read_FP32(read_buffer);
            entity_transform.angles[0] = PacketBuffer::read_FP32(read_buffer);
            entity_transform.angles[1] = PacketBuffer::read_FP32(read_buffer);
            entity_transform.angles[2] = PacketBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_transform);
            break;
        case protocol::EntityHead::ID:
            entity_head.peer = peer;
            entity_head.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            entity_head.angles[0] = PacketBuffer::read_FP32(read_buffer);
            entity_head.angles[1] = PacketBuffer::read_FP32(read_buffer);
            entity_head.angles[2] = PacketBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_head);
            break;
        case protocol::EntityVelocity::ID:
            entity_velocity.peer = peer;
            entity_velocity.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            entity_velocity.angular[0] = PacketBuffer::read_FP32(read_buffer);
            entity_velocity.angular[1] = PacketBuffer::read_FP32(read_buffer);
            entity_velocity.angular[2] = PacketBuffer::read_FP32(read_buffer);
            entity_velocity.linear[0] = PacketBuffer::read_FP32(read_buffer);
            entity_velocity.linear[1] = PacketBuffer::read_FP32(read_buffer);
            entity_velocity.linear[2] = PacketBuffer::read_FP32(read_buffer);
            globals::dispatcher.trigger(entity_velocity);
            break;
        case protocol::SpawnPlayer::ID:
            spawn_player.peer = peer;
            spawn_player.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            globals::dispatcher.trigger(spawn_player);
            break;
        case protocol::ChatMessage::ID:
            chat_message.peer = peer;
            chat_message.type = PacketBuffer::read_UI16(read_buffer);
            chat_message.message = PacketBuffer::read_string(read_buffer);
            globals::dispatcher.trigger(chat_message);
            break;
        case protocol::SetVoxel::ID:
            set_voxel.peer = peer;
            set_voxel.coord[0] = PacketBuffer::read_I64(read_buffer);
            set_voxel.coord[1] = PacketBuffer::read_I64(read_buffer);
            set_voxel.coord[2] = PacketBuffer::read_I64(read_buffer);
            set_voxel.voxel = PacketBuffer::read_UI16(read_buffer);
            set_voxel.flags = PacketBuffer::read_UI16(read_buffer);
            globals::dispatcher.trigger(set_voxel);
            break;
    }
}
