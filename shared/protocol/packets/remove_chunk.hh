// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_PACKETS_REMOVE_CHUNK_HH
#define SHARED_PROTOCOL_PACKETS_REMOVE_CHUNK_HH
#include <shared/protocol/protocol.hh>
#include <shared/coord.hh>

struct protocol::RemoveChunk final : public protocol::BasePacket<0x000C> {
    ChunkCoord chunk {};
};

template<>
inline void protocol::decode_packet<protocol::RemoveChunk>(PacketReader &reader, protocol::RemoveChunk &packet)
{
    packet.chunk[0] = PacketReader::I32(reader);
    packet.chunk[1] = PacketReader::I32(reader);
    packet.chunk[2] = PacketReader::I32(reader);
}

template<>
inline void protocol::encode_packet<protocol::RemoveChunk>(PacketWriter &writer, const protocol::RemoveChunk &packet)
{
    PacketWriter::I32(writer, packet.chunk[0]);
    PacketWriter::I32(writer, packet.chunk[1]);
    PacketWriter::I32(writer, packet.chunk[2]);
}

#endif /* SHARED_PROTOCOL_PACKETS_REMOVE_CHUNK_HH */
