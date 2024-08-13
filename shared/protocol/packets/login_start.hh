// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_LOGIN_START_HH
#define SHARED_PROTOCOL_PACKETS_LOGIN_START_HH
#include <shared/protocol/protocol.hh>

struct protocol::LoginStart final : public protocol::BasePacket<0x0002> {
    std::uint64_t player_uid {};
    std::uint64_t password_crc {};
    std::uint64_t vdef_checksum {};
    std::string username {};
};

template<>
inline void protocol::decode_packet<protocol::LoginStart>(PacketReader &reader, protocol::LoginStart &packet)
{
    packet.player_uid = PacketReader::UI64(reader);
    packet.password_crc = PacketReader::UI64(reader);
    packet.vdef_checksum = PacketReader::UI64(reader);
    packet.username = PacketReader::string(reader);
}

template<>
inline void protocol::encode_packet<protocol::LoginStart>(PacketWriter &writer, const protocol::LoginStart &packet)
{
    PacketWriter::UI64(writer, packet.player_uid);
    PacketWriter::UI64(writer, packet.password_crc);
    PacketWriter::UI64(writer, packet.vdef_checksum);
    PacketWriter::string(writer, packet.username);
}

#endif /* SHARED_PROTOCOL_PACKETS_LOGIN_START_HH */
