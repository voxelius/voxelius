// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_NET_PACKET_HH
#define SHARED_NET_PACKET_HH
#include <cstdint>

namespace net
{
template<std::uint16_t packet_id>
struct Packet { constexpr static std::uint16_t ID = packet_id; };
} // namespace net

#endif /* SHARED_NET_PACKET_HH */
