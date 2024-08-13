// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_ENTITY_TRANSFORM_HH
#define SHARED_PROTOCOL_PACKETS_ENTITY_TRANSFORM_HH
#include <entt/entity/entity.hpp>
#include <shared/math/vec3angles.hh>
#include <shared/protocol/protocol.hh>
#include <shared/coord.hh>

struct protocol::EntityTransform final : public protocol::BasePacket<0x000E> {
    entt::entity entity {};
    ChunkCoord chunk {};
    Vec3f local {};
    Vec3angles angles {};
};

template<>
inline void protocol::decode_packet<protocol::EntityTransform>(PacketReader &reader, protocol::EntityTransform &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.chunk[0] = PacketReader::I32(reader);
    packet.chunk[1] = PacketReader::I32(reader);
    packet.chunk[2] = PacketReader::I32(reader);
    packet.local[0] = PacketReader::FP32(reader);
    packet.local[1] = PacketReader::FP32(reader);
    packet.local[2] = PacketReader::FP32(reader);
    packet.angles[0] = PacketReader::FP32(reader);
    packet.angles[1] = PacketReader::FP32(reader);
    packet.angles[2] = PacketReader::FP32(reader);
}

template<>
inline void protocol::encode_packet<protocol::EntityTransform>(PacketWriter &writer, const protocol::EntityTransform &packet)
{
    PacketWriter::UI64(writer, static_cast<std::uint64_t>(packet.entity));
    PacketWriter::I32(writer, packet.chunk[0]);
    PacketWriter::I32(writer, packet.chunk[1]);
    PacketWriter::I32(writer, packet.chunk[2]);
    PacketWriter::FP32(writer, packet.local[0]);
    PacketWriter::FP32(writer, packet.local[1]);
    PacketWriter::FP32(writer, packet.local[2]);
    PacketWriter::FP32(writer, packet.angles[0]);
    PacketWriter::FP32(writer, packet.angles[1]);
    PacketWriter::FP32(writer, packet.angles[2]);
}

#endif /* SHARED_PROTOCOL_PACKETS_ENTITY_TRANSFORM_HH */
