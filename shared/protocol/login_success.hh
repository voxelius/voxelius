// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_LOGIN_SUCCESS_HH
#define SHARED_PROTOCOL_LOGIN_SUCCESS_HH
#include <shared/protocol/protocol.hh>

struct protocol::LoginSuccess final : public protocol::BasePacket<0x0004> {
    std::uint32_t session_id {};
    std::string username {};
};

template<>
static inline void protocol::decode_packet<protocol::LoginSuccess>(PacketReader &reader, protocol::LoginSuccess &packet)
{
    packet.session_id = PacketReader::UI32(reader);
    packet.username = PacketReader::string(reader);
}

template<>
static inline void protocol::encode_packet<protocol::LoginSuccess>(PacketWriter &writer, const protocol::LoginSuccess &packet)
{
    PacketWriter::UI32(writer, packet.session_id);
    PacketWriter::string(writer, packet.username);
}

#endif /* SHARED_PROTOCOL_LOGIN_SUCCESS_HH */
