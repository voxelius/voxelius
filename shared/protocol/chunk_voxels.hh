// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PROTOCOL_CHUNK_VOXELS_HH
#define SHARED_PROTOCOL_CHUNK_VOXELS_HH
#include <shared/protocol/protocol.hh>
#include <shared/chunk.hh>

struct protocol::ChunkVoxels final : public protocol::BasePacket<0x0006> {
    entt::entity entity {};
    ChunkCoord coord {};
    VoxelStorage voxels {};
};

template<>
static inline void protocol::decode_packet<protocol::ChunkVoxels>(PacketReader &reader, protocol::ChunkVoxels &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
    packet.coord[0] = PacketReader::I32(reader);
    packet.coord[1] = PacketReader::I32(reader);
    packet.coord[2] = PacketReader::I32(reader);
    
    // This is slow but the server doesn't send ChunkVoxels
    // packets that frequent (only on TP-ing and on joining)
    for(std::size_t i = 0; i < CHUNK_VOLUME; packet.voxels[i++] = PacketReader::UI16(reader));
}

template<>
static inline void protocol::encode_packet<protocol::ChunkVoxels>(PacketWriter &writer, const protocol::ChunkVoxels &packet)
{
    PacketWriter::UI64(static_cast<std::uint64_t>(packet.entity));
    PacketWriter::I32(packet.coord[0]);
    PacketWriter::I32(packet.coord[1]);
    PacketWriter::I32(packet.coord[2]);

    // This is slow but the server doesn't send ChunkVoxels
    // packets that frequent (only on TP-ing and on joining)
    for(std::size_t i = 0; i < CHUNK_VOLUME; PacketWriter::UI16(packet.voxels[i++]));
}

#endif /* SHARED_PROTOCOL_CHUNK_VOXELS_HH */
