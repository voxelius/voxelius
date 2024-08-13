// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_RWBUFFER_HH
#define SHARED_RWBUFFER_HH
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include <vector>

struct RWBuffer final {
    std::size_t read_position {};
    std::vector<std::uint8_t> vector {};
    
public:
    static float read_FP32(RWBuffer &buffer);
    static std::int8_t read_I8(RWBuffer &buffer);
    static std::int16_t read_I16(RWBuffer &buffer);
    static std::int32_t read_I32(RWBuffer &buffer);
    static std::int64_t read_I64(RWBuffer &buffer);
    static std::uint8_t read_UI8(RWBuffer &buffer);
    static std::uint16_t read_UI16(RWBuffer &buffer);
    static std::uint32_t read_UI32(RWBuffer &buffer);
    static std::uint64_t read_UI64(RWBuffer &buffer);
    static std::string read_string(RWBuffer &buffer);
    
public:
    static void write_FP32(RWBuffer &buffer, float value);
    static void write_I8(RWBuffer &buffer, std::int8_t value);
    static void write_I16(RWBuffer &buffer, std::int16_t value);
    static void write_I32(RWBuffer &buffer, std::int32_t value);
    static void write_I64(RWBuffer &buffer, std::int64_t value);
    static void write_UI8(RWBuffer &buffer, std::uint8_t value);
    static void write_UI16(RWBuffer &buffer, std::uint16_t value);
    static void write_UI32(RWBuffer &buffer, std::uint32_t value);
    static void write_UI64(RWBuffer &buffer, std::uint64_t value);
    static void write_string(RWBuffer &buffer, const std::string &value);

public:
    static void setup(RWBuffer &buffer);
    static void setup(RWBuffer &buffer, const void *data, std::size_t size);
    static ENetPacket *submit(const RWBuffer &buffer, ENetPacketFlag flags);
};

#endif /* SHARED_RWBUFFER_HH */
