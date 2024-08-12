// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_NET_HANDSHAKE_HH
#define SHARED_NET_HANDSHAKE_HH
#include <cstdint>

struct PacketReader;
struct PacketWriter;

namespace net
{
struct Handshake final {
    constexpr static std::uint16_t ID = 0x0000;
    constexpr static std::uint16_t PING = 0x00000000;
    constexpr static std::uint16_t LOGIN = 0x00000001;

public:
    std::uint32_t version {};
    std::uint32_t intention {};

public:
    static void decode(net::Handshake &packet, PacketReader &reader);
    static void encode(const net::Handshake &packet, PacketWriter &writer);
};
} // namespace net

#endif /* SHARED_NET_HANDSHAKE_HH */
