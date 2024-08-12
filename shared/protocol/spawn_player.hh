// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_SPAWN_PLAYER_HH
#define SHARED_PROTOCOL_SPAWN_PLAYER_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>

struct protocol::SpawnPlayer final : public protocol::BasePacket<0x000C> {
    entt::entity entity {};
};

template<>
static inline void protocol::decode_packet<protocol::SpawnPlayer>(PacketReader &reader, protocol::SpawnPlayer &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
}

template<>
static inline void protocol::encode_packet<protocol::SpawnPlayer>(PacketWriter &writer, const protocol::SpawnPlayer &packet)
{
    PacketWriter::UI64(static_cast<std::uint64_t>(packet.entity));
}

#endif /* SHARED_PROTOCOL_SPAWN_PLAYER_HH */
