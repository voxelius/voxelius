// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_DISCONNECT_HH
#define SHARED_PROTOCOL_DISCONNECT_HH
#include <shared/protocol/protocol.hh>

struct protocol::Disconnect final : public protocol::BasePacket<0x0003> {
    std::string reason {};
};

template<>
static inline void protocol::decode_packet<protocol::Disconnect>(PacketReader &reader, protocol::Disconnect &packet)
{
    packet.reason = PacketReader::string(reader);
}

template<>
static inline void protocol::encode_packet<protocol::Disconnect>(PacketWriter &writer, const protocol::Disconnect &packet)
{
    PacketWriter::string(packet.reason);
}

#endif /* SHARED_PROTOCOL_DISCONNECT_HH */
