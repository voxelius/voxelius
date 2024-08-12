// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PACKET_READER_HH
#define SHARED_PACKET_READER_HH
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include <vector>

struct PacketWriter;
struct PacketReader final {
    std::size_t read_position {};
    std::vector<std::uint8_t> buffer {};

public:
    static void setup(PacketReader &reader, const ENetPacket *packet);
    static void setup(PacketReader &reader, const void *data, std::size_t size);
    static void setup(PacketReader &reader, const PacketWriter &writer);

public:
    static std::int8_t I8(PacketReader &reader);
    static std::int16_t I16(PacketReader &reader);
    static std::int32_t I32(PacketReader &reader);
    static std::int64_t I64(PacketReader &reader);

public:
    static std::uint8_t UI8(PacketReader &reader);
    static std::uint16_t UI16(PacketReader &reader);
    static std::uint32_t UI32(PacketReader &reader);
    static std::uint64_t UI64(PacketReader &reader);

public:
    static std::string string(PacketReader &reader);

public:
    static float FP32(PacketReader &reader);
};

#endif /* SHARED_PACKET_READER_HH */
