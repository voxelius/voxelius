// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_REMOVE_ENTITY_HH
#define SHARED_PROTOCOL_REMOVE_ENTITY_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>

struct protocol::RemoveEntity final : public protocol::BasePacket<0x000B> {
    entt::entity entity {};
    std::uint32_t flags {};
    
public:
    constexpr static std::uint32_t PLAY_DEATHANIM = 0x00000001;
};

template<>
static inline void protocol::decode_packet<protocol::RemoveEntity>(PacketReader &reader, protocol::RemoveEntity &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.flags = PacketReader::UI32(reader);
}

template<>
static inline void protocol::encode_packet<protocol::RemoveEntity>(PacketWriter &writer, const protocol::RemoveEntity &packet)
{
    PacketWriter::UI64(static_cast<std::uint64_t>(packet.entity));
    PacketWriter::UI32(packet.flags);
}

#endif /* SHARED_PROTOCOL_REMOVE_ENTITY_HH */
