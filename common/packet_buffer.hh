// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct PacketBuffer final {
    std::size_t read_position {};
    std::vector<std::uint8_t> vector {};
    
public:
    static float read_FP32(PacketBuffer &buffer);
    static std::int8_t read_I8(PacketBuffer &buffer);
    static std::int16_t read_I16(PacketBuffer &buffer);
    static std::int32_t read_I32(PacketBuffer &buffer);
    static std::int64_t read_I64(PacketBuffer &buffer);
    static std::uint8_t read_UI8(PacketBuffer &buffer);
    static std::uint16_t read_UI16(PacketBuffer &buffer);
    static std::uint32_t read_UI32(PacketBuffer &buffer);
    static std::uint64_t read_UI64(PacketBuffer &buffer);
    static std::string read_string(PacketBuffer &buffer);
    
public:
    static void write_FP32(PacketBuffer &buffer, float value);
    static void write_I8(PacketBuffer &buffer, std::int8_t value);
    static void write_I16(PacketBuffer &buffer, std::int16_t value);
    static void write_I32(PacketBuffer &buffer, std::int32_t value);
    static void write_I64(PacketBuffer &buffer, std::int64_t value);
    static void write_UI8(PacketBuffer &buffer, std::uint8_t value);
    static void write_UI16(PacketBuffer &buffer, std::uint16_t value);
    static void write_UI32(PacketBuffer &buffer, std::uint32_t value);
    static void write_UI64(PacketBuffer &buffer, std::uint64_t value);
    static void write_string(PacketBuffer &buffer, const std::string &value);

public:
    static void setup(PacketBuffer &buffer);
    static void setup(PacketBuffer &buffer, const void *data, std::size_t size);
};
