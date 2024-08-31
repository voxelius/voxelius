// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/packet_buffer.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/globals.hh>
#include <game/shared/protocol.hh>
#include <mathlib/floathacks.hh>
#include <miniz.h>

static PacketBuffer read_buffer = {};
static PacketBuffer write_buffer = {};
static std::vector<std::uint8_t> read_zdata = {};
static std::vector<std::uint8_t> write_zdata = {};

static void write_voxel_storage(PacketBuffer &buffer, const VoxelStorage &storage)
{
    mz_ulong bound = mz_compressBound(sizeof(VoxelStorage));

    VoxelStorage net_storage = {};
    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        // Convert voxel data into network byte order
        // FIXME: what if we change voxel size to 32 bits?
        net_storage[i] = ENET_HOST_TO_NET_16(storage[i]);
    }

    write_zdata.resize(bound);
    mz_compress(write_zdata.data(), &bound, reinterpret_cast<const unsigned char *>(net_storage.data()), sizeof(VoxelStorage));
    PacketBuffer::write_UI64(buffer, static_cast<std::uint64_t>(bound));
    for(mz_ulong i = 0; i < bound; PacketBuffer::write_UI8(buffer, write_zdata[i++]));
}

static void read_voxel_storage(PacketBuffer &buffer, VoxelStorage &storage)
{
    mz_ulong size = static_cast<mz_ulong>(sizeof(VoxelStorage));
    mz_ulong bound = static_cast<mz_ulong>(PacketBuffer::read_UI64(buffer));

    read_zdata.resize(bound);
    for(mz_ulong i = 0; i < bound; read_zdata[i++] = PacketBuffer::read_UI8(buffer));
    mz_uncompress(reinterpret_cast<unsigned char *>(storage.data()), &size, read_zdata.data(), bound);

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        // Convert voxel storage to host byte order in-situ
        // FIXME: what if we change voxel size to 32 bits?
        storage[i] = ENET_NET_TO_HOST_16(storage[i]);
    }
}

// [peer], [NULL] - send to one specific peer
// [NULL], [host] - broadcast to all the host peers
// [peer], [host] - broadcast to all the peers except one
static void basic_send(ENetPeer *peer, ENetHost *host, ENetPacket *packet)
{
    if(host) {
        for(std::size_t i = 0; i < host->peerCount; ++i) {
            if(host->peers[i].state == ENET_PEER_STATE_CONNECTED) {
                if(&host->peers[i] == peer)
                    continue;
                enet_peer_send(&host->peers[i], 0, packet);
            }
        }

        if(packet->referenceCount == 0) {
            // ENet seems to do that as well
            enet_packet_destroy(packet);
        }
    }
    else if(peer) {
        // Send to just one peer
        enet_peer_send(peer, 0, packet);
    }
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::StatusRequest &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::StatusRequest::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::StatusResponse &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::StatusResponse::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    PacketBuffer::write_UI16(write_buffer, packet.max_players);
    PacketBuffer::write_UI16(write_buffer, packet.num_players);
    PacketBuffer::write_string(write_buffer, packet.motd);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::LoginRequest &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::LoginRequest::ID);
    PacketBuffer::write_UI32(write_buffer, packet.version);
    PacketBuffer::write_UI64(write_buffer, packet.password_hash);
    PacketBuffer::write_UI64(write_buffer, packet.vdef_checksum);
    PacketBuffer::write_UI64(write_buffer, packet.player_uid);
    PacketBuffer::write_string(write_buffer, packet.username);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::LoginResponse &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::LoginResponse::ID);
    PacketBuffer::write_UI16(write_buffer, packet.session_id);
    PacketBuffer::write_UI16(write_buffer, packet.tickrate);
    PacketBuffer::write_string(write_buffer, packet.username);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::Disconnect &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::Disconnect::ID);
    PacketBuffer::write_string(write_buffer, packet.reason);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::ChunkVoxels &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::ChunkVoxels::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_I32(write_buffer, packet.chunk[0]);
    PacketBuffer::write_I32(write_buffer, packet.chunk[1]);
    PacketBuffer::write_I32(write_buffer, packet.chunk[2]);
    write_voxel_storage(write_buffer, packet.voxels);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::EntityTransform &packet)
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
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::EntityHead &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::EntityHead::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    PacketBuffer::write_FP32(write_buffer, packet.angles[0]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[1]);
    PacketBuffer::write_FP32(write_buffer, packet.angles[2]);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::EntityVelocity &packet)
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
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::SpawnPlayer &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::SpawnPlayer::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));    
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::ChatMessage &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::ChatMessage::ID);
    PacketBuffer::write_UI16(write_buffer, packet.type);
    PacketBuffer::write_string(write_buffer, packet.message);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::SetVoxel &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::SetVoxel::ID);
    PacketBuffer::write_I64(write_buffer, packet.coord[0]);
    PacketBuffer::write_I64(write_buffer, packet.coord[1]);
    PacketBuffer::write_I64(write_buffer, packet.coord[2]);
    PacketBuffer::write_UI16(write_buffer, packet.voxel);
    PacketBuffer::write_UI16(write_buffer, packet.flags);
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::RemoveEntity &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::RemoveEntity::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::send(ENetPeer *peer, ENetHost *host, const protocol::EntityPlayer &packet)
{
    PacketBuffer::setup(write_buffer);
    PacketBuffer::write_UI16(write_buffer, protocol::EntityPlayer::ID);
    PacketBuffer::write_UI64(write_buffer, static_cast<std::uint64_t>(packet.entity));
    basic_send(peer, host, enet_packet_create(write_buffer.vector.data(), write_buffer.vector.size(), ENET_PACKET_FLAG_RELIABLE));
}

void protocol::receive(const ENetPacket *packet, ENetPeer *peer)
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
    protocol::RemoveEntity remove_entity = {};
    protocol::EntityPlayer entity_player = {};
    
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
            status_response.motd = PacketBuffer::read_string(read_buffer);
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
            read_voxel_storage(read_buffer, chunk_voxels.voxels);
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
        case protocol::RemoveEntity::ID:
            remove_entity.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            globals::dispatcher.trigger(remove_entity);
            break;
        case protocol::EntityPlayer::ID:
            entity_player.entity = static_cast<entt::entity>(PacketBuffer::read_UI64(read_buffer));
            globals::dispatcher.trigger(entity_player);
            break;
    }
}

void protocol::send_disconnect(ENetPeer *peer, ENetHost *host, const std::string &reason)
{
    protocol::Disconnect packet = {};
    packet.reason = reason;
    protocol::send(peer, host, packet);
}

void protocol::send_chat_message(ENetPeer *peer, ENetHost *host, const std::string &message)
{
    protocol::ChatMessage packet = {};
    packet.type = UINT16_C(0x0000); // UNDONE
    packet.message = message;
    protocol::send(peer, host, packet);
}

void protocol::send_chunk_voxels(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    if(const ChunkComponent *component = globals::registry.try_get<ChunkComponent>(entity)) {
        protocol::ChunkVoxels packet = {};
        packet.entity = entity;
        packet.chunk = component->coord;
        packet.voxels = component->chunk->voxels;
        protocol::send(peer, host, packet);
    }
}

void protocol::send_entity_head(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    if(const HeadComponent *component = globals::registry.try_get<HeadComponent>(entity)) {
        protocol::EntityHead packet = {};
        packet.entity = entity;
        packet.angles = component->angles;
        protocol::send(peer, host, packet);
    }
}

void protocol::send_entity_transform(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    if(const TransformComponent *component = globals::registry.try_get<TransformComponent>(entity)) {
        protocol::EntityTransform packet = {};
        packet.entity = entity;
        packet.coord = component->position;
        packet.angles = component->angles;
        protocol::send(peer, host, packet);
    }
}

void protocol::send_entity_velocity(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    if(const VelocityComponent *component = globals::registry.try_get<VelocityComponent>(entity)) {
        protocol::EntityVelocity packet = {};
        packet.entity = entity;
        packet.angular = component->angular;
        packet.linear = component->linear;
        protocol::send(peer, host, packet);
    }
}

void protocol::send_entity_player(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    protocol::EntityPlayer packet = {};
    packet.entity = entity;
    protocol::send(peer, host, packet);
}

void protocol::send_spawn_player(ENetPeer *peer, ENetHost *host, entt::entity entity)
{
    protocol::SpawnPlayer packet = {};
    packet.entity = entity;
    protocol::send(peer, host, packet);
}

void protocol::send_set_voxel(ENetPeer *peer, ENetHost *host, const VoxelCoord &vpos, Voxel voxel)
{
    protocol::SetVoxel packet = {};
    packet.coord = vpos;
    packet.voxel = voxel;
    packet.flags = UINT16_C(0x0000); // UNDONE
    protocol::send(peer, host, packet);
}
