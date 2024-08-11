// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_PACKET_HH
#define SHARED_PACKET_HH
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include <vector>

class Packet final {
public:
    std::size_t read_position {};
    std::vector<std::uint8_t> buffer {};

public:
    static void setup(Packet &packet);
    static void setup(Packet &packet, const ENetPacket *source);
    static ENetPacket *submit(const Packet &packet, ENetPacketFlag flags);

public:
    static void write_F32(Packet &packet, float value);
    static void write_I8(Packet &packet, std::int8_t value);
    static void write_I16(Packet &packet, std::int16_t value);
    static void write_I32(Packet &packet, std::int32_t value);
    static void write_I64(Packet &packet, std::int64_t value);
    static void write_UI8(Packet &packet, std::uint8_t value);
    static void write_UI16(Packet &packet, std::uint16_t value);
    static void write_UI32(Packet &packet, std::uint32_t value);
    static void write_UI64(Packet &packet, std::uint64_t value);
    static void write_string(Packet &packet, const std::string &value);

public:
    static float read_F32(Packet &packet);
    static std::int8_t read_I8(Packet &packet);
    static std::int16_t read_I16(Packet &packet);
    static std::int32_t read_I32(Packet &packet);
    static std::int64_t read_I64(Packet &packet);
    static std::uint8_t read_UI8(Packet &packet);
    static std::uint16_t read_UI16(Packet &packet);
    static std::uint32_t read_UI32(Packet &packet);
    static std::uint64_t read_UI64(Packet &packet);
    static std::string read_string(Packet &packet);
};

#endif /* SHARED_PACKET_HH */
