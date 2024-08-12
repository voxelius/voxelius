// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_ENTITY_VELOCITY_HH
#define SHARED_PROTOCOL_ENTITY_VELOCITY_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>
#include <shared/coord.hh>

struct protocol::EntityVelocity final : public protocol::BasePacket<0x000A> {
    entt::entity entity {};
    Vec3f linear {};
};

template<>
static inline void protocol::decode_packet<protocol::EntityVelocity>(PacketReader &reader, protocol::EntityVelocity &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.linear[0] = PacketReader::FP32(reader);
    packet.linear[1] = PacketReader::FP32(reader);
    packet.linear[2] = PacketReader::FP32(reader);
}

template<>
static inline void protocol::encode_packet<protocol::EntityVelocity>(PacketWriter &writer, const protocol::EntityVelocity &packet)
{
    PacketWriter::UI64(static_cast<std::uint64_t>(packet.entity));
    PacketWriter::FP32(packet.linear[0]);
    PacketWriter::FP32(packet.linear[1]);
    PacketWriter::FP32(packet.linear[2]);
}

#endif /* SHARED_PROTOCOL_ENTITY_VELOCITY_HH */
