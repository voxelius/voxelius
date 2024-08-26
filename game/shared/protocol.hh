// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <mathlib/vec3angles.hh>
#include <game/shared/chunk.hh>

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
ENetPacket *make_packet(const StatusRequest &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const StatusResponse &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const LoginRequest &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const LoginResponse &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const Disconnect &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const ChunkVoxels &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const EntityTransform &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const EntityHead &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const EntityVelocity &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const SpawnPlayer &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const ChatMessage &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
ENetPacket *make_packet(const SetVoxel &packet, std::uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
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