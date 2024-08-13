// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_HANDSHAKE_HH
#define SHARED_PROTOCOL_PACKETS_HANDSHAKE_HH
#include <shared/protocol/protocol.hh>

struct protocol::Handshake final : public protocol::BasePacket<0x0000> {
    std::uint32_t intention {};
    std::uint32_t version {};

public:
    constexpr static std::uint32_t PING = 0;
    constexpr static std::uint32_t LOGIN = 1;
};

template<>
inline void protocol::decode_packet<protocol::Handshake>(PacketReader &reader, protocol::Handshake &packet)
{
    packet.version = PacketReader::UI32(reader);
    packet.intention = PacketReader::UI32(reader);
}

template<>
inline void protocol::encode_packet<protocol::Handshake>(PacketWriter &writer, const protocol::Handshake &packet)
{
    PacketWriter::UI32(writer, packet.version);
    PacketWriter::UI32(writer, packet.intention);
}

#endif /* SHARED_PROTOCOL_PACKETS_HANDSHAKE_HH */
