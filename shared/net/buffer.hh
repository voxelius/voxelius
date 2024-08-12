// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_NET_BUFFER_HH
#define SHARED_NET_BUFFER_HH
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include <vector>

namespace net
{
class Buffer final {
public:
    std::size_t read_position {};
    std::vector<std::uint8_t> vector {};

public:
    static void setup(net::Buffer &buffer);
    static void setup(net::Buffer &buffer, const ENetPacket *source);
    static ENetPacket *submit(const net::Buffer &buffer, ENetPacketFlag flags);

public:
    static void write_F32(net::Buffer &buffer, float value);
    static void write_I8(net::Buffer &buffer, std::int8_t value);
    static void write_I16(net::Buffer &buffer, std::int16_t value);
    static void write_I32(net::Buffer &buffer, std::int32_t value);
    static void write_I64(net::Buffer &buffer, std::int64_t value);
    static void write_UI8(net::Buffer &buffer, std::uint8_t value);
    static void write_UI16(net::Buffer &buffer, std::uint16_t value);
    static void write_UI32(net::Buffer &buffer, std::uint32_t value);
    static void write_UI64(net::Buffer &buffer, std::uint64_t value);
    static void write_string(net::Buffer &buffer, const std::string &value);

public:
    static float read_F32(net::Buffer &buffer);
    static std::int8_t read_I8(net::Buffer &buffer);
    static std::int16_t read_I16(net::Buffer &buffer);
    static std::int32_t read_I32(net::Buffer &buffer);
    static std::int64_t read_I64(net::Buffer &buffer);
    static std::uint8_t read_UI8(net::Buffer &buffer);
    static std::uint16_t read_UI16(net::Buffer &buffer);
    static std::uint32_t read_UI32(net::Buffer &buffer);
    static std::uint64_t read_UI64(net::Buffer &buffer);
    static std::string read_string(net::Buffer &buffer);
};
} // namespace net

#endif /* SHARED_NET_BUFFER_HH */
