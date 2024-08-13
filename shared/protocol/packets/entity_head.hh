// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_ENTITY_HEAD_HH
#define SHARED_PROTOCOL_PACKETS_ENTITY_HEAD_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>
#include <shared/math/vec3angles.hh>

struct protocol::EntityHead final : public protocol::BasePacket<0x000F> {
    entt::entity entity {};
    Vec3angles angles {};
};

template<>
inline void protocol::decode_packet<protocol::EntityHead>(PacketReader &reader, protocol::EntityHead &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.angles[0] = PacketReader::FP32(reader);
    packet.angles[1] = PacketReader::FP32(reader);
    packet.angles[2] = PacketReader::FP32(reader);
}

template<>
inline void protocol::encode_packet<protocol::EntityHead>(PacketWriter &writer, const protocol::EntityHead &packet)
{
    PacketWriter::UI64(writer, static_cast<std::uint64_t>(packet.entity));
    PacketWriter::FP32(writer, packet.angles[0]);
    PacketWriter::FP32(writer, packet.angles[1]);
    PacketWriter::FP32(writer, packet.angles[2]);
}

#endif /* SHARED_PROTOCOL_PACKETS_ENTITY_HEAD_HH */
