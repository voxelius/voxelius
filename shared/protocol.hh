// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_HH
#define SHARED_PROTOCOL_HH
#include <cstdint>
#include <enet/enet.h>
#include <shared/math/vec3angles.hh>
#include <shared/chunk.hh>

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
struct CreateEntity;
struct ChunkVoxels;
struct EntityTransform;
struct EntityHead;
struct EntityVelocity;
struct SpawnPlayer;
struct ChatMessage;
struct SetVoxel;
} // namespace protocol

namespace protocol
{
void send_packet(ENetPeer *peer, const StatusRequest &packet);
void send_packet(ENetPeer *peer, const StatusResponse &packet);
void send_packet(ENetPeer *peer, const LoginRequest &packet);
void send_packet(ENetPeer *peer, const LoginResponse &packet);
void send_packet(ENetPeer *peer, const Disconnect &packet);
void send_packet(ENetPeer *peer, const CreateEntity &packet);
void send_packet(ENetPeer *peer, const ChunkVoxels &packet);
void send_packet(ENetPeer *peer, const EntityTransform &packet);
void send_packet(ENetPeer *peer, const EntityHead &packet);
void send_packet(ENetPeer *peer, const EntityVelocity &packet);
void send_packet(ENetPeer *peer, const SpawnPlayer &packet);
void send_packet(ENetPeer *peer, const ChatMessage &packet);
void send_packet(ENetPeer *peer, const SetVoxel &packet);
} // namespace protocol

namespace protocol
{
void handle_packet(const ENetPacket *packet, ENetPeer *peer);
} // namespace protocol

struct protocol::StatusRequest final : public protocol::Base<0x0000> {
    std::uint32_t version {};
};

struct protocol::StatusResponse final : public protocol::Base<0x0001> {
    std::uint32_t version {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
};

struct protocol::LoginRequest final : public protocol::Base<0x0002> {
    std::uint32_t version {};
    std::uint64_t password_hash {};
    std::uint64_t vdef_checksum {};
    std::uint64_t player_userid {};
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

struct protocol::CreateEntity final : public protocol::Base<0x0005> {
    entt::entity entity {};
    std::uint32_t type {};
};

struct protocol::ChunkVoxels final : public protocol::Base<0x0006> {
    entt::entity entity {};
    ChunkCoord chunk {};
    VoxelStorage voxels {};
};

struct protocol::EntityTransform final : public protocol::Base<0x0007> {
    entt::entity entity {};
    WorldCoord coord {};
    Vec3angles angles {};
};

struct protocol::EntityHead final : public protocol::Base<0x0008> {
    entt::entity entity {};
    Vec3angles angles {};
};

struct protocol::EntityVelocity final : public protocol::Base<0x0009> {
    entt::entity entity {};
    Vec3f value {};
};

struct protocol::SpawnPlayer final : public protocol::Base<0x000A> {
    entt::entity entity {};
};

struct protocol::ChatMessage final : public protocol::Base<0x000B> {
    std::uint16_t type {};
    std::string message {};
};

struct protocol::SetVoxel final : public protocol::Base<0x000C> {
    VoxelCoord coord {};
    Voxel voxel {};
    std::uint16_t flags {};
};

#endif /* SHARED_PROTOCOL_HH */
