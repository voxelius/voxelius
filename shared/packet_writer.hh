// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PACKET_WRITER_HH
#define SHARED_PACKET_WRITER_HH
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include <vector>

struct PacketWriter final {
    std::vector<std::uint8_t> buffer {};
    
public:
    static ENetPacket *submit(const PacketWriter &writer, ENetPacketFlag flags);

public:
    static void I8(PacketWriter &writer, std::int8_t value);
    static void I16(PacketWriter &writer, std::int16_t value);
    static void I32(PacketWriter &writer, std::int32_t value);
    static void I64(PacketWriter &writer, std::int64_t value);

public:
    static void UI8(PacketWriter &writer, std::uint8_t value);
    static void UI16(PacketWriter &writer, std::uint16_t value);
    static void UI32(PacketWriter &writer, std::uint32_t value);
    static void UI64(PacketWriter &writer, std::uint64_t value);

public:
    static void string(PacketWriter &writer, const std::string &value);

public:
    static void FP32(PacketWriter &writer, float value);
};

#endif /* SHARED_PACKET_WRITER_HH */
