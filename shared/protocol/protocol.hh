// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PROTOCOL_HH
#define SHARED_PROTOCOL_PROTOCOL_HH
#include <enet/enet.h>
#include <shared/packet_reader.hh>
#include <shared/packet_writer.hh>

namespace protocol
{
constexpr static std::uint32_t VERSION = 0x00000002;
}

namespace protocol
{
template<std::uint16_t packet_id>
struct BasePacket {
    constexpr static std::uint16_t ID = packet_id;
    ENetPeer *peer {nullptr};
};
} // namespace protocol

namespace protocol
{
struct Handshake;
struct ServerInfo;
struct LoginStart;
struct Disconnect;
struct LoginSuccess;
struct ChunkVoxels;
struct AddEntity;
struct EntityAngles;
struct EntityPosition;
struct EntityAngVel;
struct EntityVelocity;
struct RemoveEntity;
struct RemoveChunk;
struct SpawnPlayer;
} // namespace protocol

namespace protocol
{
template<typename packet_type>
void decode_packet(PacketReader &reader, packet_type &packet);
template<typename packet_type>
void encode_packet(PacketWriter &writer, const packet_type &packet);
} // namespace protocol

#endif /* SHARED_PROTOCOL_PROTOCOL_HH */