// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_ENTITY_ANGVEL_HH
#define SHARED_PROTOCOL_PACKETS_ENTITY_ANGVEL_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>
#include <shared/math/vec3angles.hh>

struct protocol::EntityAngVel final : public protocol::BasePacket<0x0009> {
    entt::entity entity {};
    Vec3angles angular {};
};

template<>
inline void protocol::decode_packet<protocol::EntityAngVel>(PacketReader &reader, protocol::EntityAngVel &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.angular[0] = PacketReader::FP32(reader);
    packet.angular[1] = PacketReader::FP32(reader);
    packet.angular[2] = PacketReader::FP32(reader);
}

template<>
inline void protocol::encode_packet<protocol::EntityAngVel>(PacketWriter &writer, const protocol::EntityAngVel &packet)
{
    PacketWriter::UI64(writer, static_cast<std::uint64_t>(packet.entity));
    PacketWriter::FP32(writer, packet.angular[0]);
    PacketWriter::FP32(writer, packet.angular[1]);
    PacketWriter::FP32(writer, packet.angular[2]);
}

#endif /* SHARED_PROTOCOL_PACKETS_ENTITY_ANGVEL_HH */
