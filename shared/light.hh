// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_LIGHT_HH
#define SHARED_LIGHT_HH
#include <cstdint>

using Light = std::uint8_t;
constexpr static Light LIGHT_SKY_BITS = 0x0F;
constexpr static Light LIGHT_VOX_BITS = 0xF0;
constexpr static Light NULL_LIGHT = 0x00;

#endif /* SHARED_LIGHT_HH */
