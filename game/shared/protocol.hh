// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <mathlib/vec3angles.hh>
#include <game/shared/chunk.hh>
#include <game/shared/world_coord.hh>

namespace protocol
{
constexpr static std::uint16_t PORT = 43103;
constexpr static std::uint32_t VERSION = 2;
} // namespace protocol

namespace protocol
{
template<std::uint16_t packet_id>
struct Base {
    constexpr static std::uint16_t ID = packet_id;
    virtual ~Base(void) = default;
    ENetPeer *peer {nullptr};
};
} // namespace protocol

namespace protocol
{
struct StatusRequest;
struct StatusResponse;
struct LoginRequest;
struct LoginResponse;
struct Disconnect;
struct ChunkVoxels;
struct EntityTransform;
struct EntityHead;
struct EntityVelocity;
struct SpawnPlayer;
struct ChatMessage;
struct SetVoxel;
struct RemoveEntity;
struct EntityPlayer;
} // namespace protocol

namespace protocol
{
void send(ENetPeer *peer, ENetHost *host, const StatusRequest &packet);
void send(ENetPeer *peer, ENetHost *host, const StatusResponse &packet);
void send(ENetPeer *peer, ENetHost *host, const LoginRequest &packet);
void send(ENetPeer *peer, ENetHost *host, const LoginResponse &packet);
void send(ENetPeer *peer, ENetHost *host, const Disconnect &packet);
void send(ENetPeer *peer, ENetHost *host, const ChunkVoxels &packet);
void send(ENetPeer *peer, ENetHost *host, const EntityTransform &packet);
void send(ENetPeer *peer, ENetHost *host, const EntityHead &packet);
void send(ENetPeer *peer, ENetHost *host, const EntityVelocity &packet);
void send(ENetPeer *peer, ENetHost *host, const SpawnPlayer &packet);
void send(ENetPeer *peer, ENetHost *host, const ChatMessage &packet);
void send(ENetPeer *peer, ENetHost *host, const SetVoxel &packet);
void send(ENetPeer *peer, ENetHost *host, const RemoveEntity &packet);
void send(ENetPeer *peer, ENetHost *host, const EntityPlayer &packet);
} // namespace protocol

namespace protocol
{
void receive(const ENetPacket *packet, ENetPeer *peer);
} // namespace protocol

namespace protocol
{
void send_disconnect(ENetPeer *peer, ENetHost *host, const std::string &reason);
void send_chat_message(ENetPeer *peer, ENetHost *host, const std::string &message);
} // namespace protocol

namespace protocol
{
void send_chunk_voxels(ENetPeer *peer, ENetHost *host, entt::entity entity);
void send_entity_head(ENetPeer *peer, ENetHost *host, entt::entity entity);
void send_entity_transform(ENetPeer *peer, ENetHost *host, entt::entity entity);
void send_entity_velocity(ENetPeer *peer, ENetHost *host, entt::entity entity);
void send_entity_player(ENetPeer *peer, ENetHost *host, entt::entity entity);
void send_spawn_player(ENetPeer *peer, ENetHost *host, entt::entity entity);
} // namespace protocol

namespace protocol
{
void send_set_voxel(ENetPeer *peer, ENetHost *host, const VoxelCoord &vpos, Voxel voxel);
} // namespace protocol

struct protocol::StatusRequest final : public protocol::Base<0x0000> {
    std::uint32_t version {};
};

struct protocol::StatusResponse final : public protocol::Base<0x0001> {
    std::uint32_t version {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
    std::string motd {};
};

struct protocol::LoginRequest final : public protocol::Base<0x0002> {
    std::uint32_t version {};
    std::uint64_t password_hash {};
    std::uint64_t vdef_checksum {};
    std::uint64_t player_uid {};
    std::string username {};
};

struct protocol::LoginResponse final : public protocol::Base<0x0003> {
    std::uint16_t session_id {};
    std::uint16_t tickrate {};
    std::string username {};
};

struct protocol::Disconnect final : public protocol::Base<0x0004> {
    std::string reason {};
};

struct protocol::ChunkVoxels final : public protocol::Base<0x0005> {
    entt::entity entity {};
    ChunkCoord chunk {};
    VoxelStorage voxels {};
};

struct protocol::EntityTransform final : public protocol::Base<0x0006> {
    entt::entity entity {};
    WorldCoord coord {};
    Vec3angles angles {};
};

struct protocol::EntityHead final : public protocol::Base<0x0007> {
    entt::entity entity {};
    Vec3angles angles {};
};

struct protocol::EntityVelocity final : public protocol::Base<0x0008> {
    entt::entity entity {};
    Vec3angles angular {};
    Vec3f linear {};
};

struct protocol::SpawnPlayer final : public protocol::Base<0x0009> {
    entt::entity entity {};
};

struct protocol::ChatMessage final : public protocol::Base<0x000A> {
    std::uint16_t type {};
    std::string message {};
};

struct protocol::SetVoxel final : public protocol::Base<0x000B> {
    VoxelCoord coord {};
    Voxel voxel {};
    std::uint16_t flags {};
};

struct protocol::RemoveEntity final : public protocol::Base<0x000C> {
    entt::entity entity {};
};

struct protocol::EntityPlayer final : public protocol::Base<0x000D> {
    entt::entity entity {};
};
