// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_ENTITY_POSITION_HH
#define SHARED_PROTOCOL_ENTITY_POSITION_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>
#include <shared/coord.hh>

struct protocol::EntityPosition final : public protocol::BasePacket<0x0008> {
    entt::entity entity {};
    ChunkCoord chunk {};
    Vec3f local {};
};

template<>
static inline void protocol::decode_packet<protocol::EntityPosition>(PacketReader &reader, protocol::EntityPosition &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.chunk[0] = PacketReader::I32(reader);
    packet.chunk[1] = PacketReader::I32(reader);
    packet.chunk[2] = PacketReader::I32(reader);
    packet.local[0] = PacketReader::FP32(reader);
    packet.local[1] = PacketReader::FP32(reader);
    packet.local[2] = PacketReader::FP32(reader);
}

template<>
static inline void protocol::encode_packet<protocol::EntityPosition>(PacketWriter &writer, const protocol::EntityPosition &packet)
{
    PacketWriter::UI64(static_cast<std::uint64_t>(packet.entity));
    PacketWriter::I32(packet.chunk[0]);
    PacketWriter::I32(packet.chunk[1]);
    PacketWriter::I32(packet.chunk[2]);
    PacketWriter::FP32(packet.local[0]);
    PacketWriter::FP32(packet.local[1]);
    PacketWriter::FP32(packet.local[2]);
}

#endif /* SHARED_PROTOCOL_ENTITY_POSITION_HH */
