// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_SERVER_INFO_HH
#define SHARED_PROTOCOL_SERVER_INFO_HH
#include <shared/protocol/protocol.hh>

struct protocol::ServerInfo final : public protocol::BasePacket<0x0001> {
    std::uint32_t version {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
    std::uint32_t flags {};
    
public:
    constexpr static std::uint32_t PASSWORD = 0x00000001;
};

template<>
static inline void protocol::decode_packet<protocol::ServerInfo>(PacketReader &reader, protocol::ServerInfo &packet)
{
    packet.version = PacketReader::UI32(reader);
    packet.max_players = PacketReader::UI16(reader);
    packet.num_players = PacketReader::UI16(reader);
    packet.flags = PacketReader::UI32(reader);
}

template<>
static inline void protocol::encode_packet<protocol::ServerInfo>(PacketWriter &writer, const protocol::ServerInfo &packet)
{
    PacketWriter::UI32(writer, packet.version);
    PacketWriter::UI16(writer, packet.max_players);
    PacketWriter::UI16(writer, packet.num_players);
    PacketWriter::UI32(writer, packet.flags);
}

#endif /* SHARED_PROTOCOL_SERVER_INFO_HH */
