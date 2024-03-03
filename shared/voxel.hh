// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef SHARED_VOXEL_HH
#define SHARED_VOXEL_HH
#include <stdint.h>

using voxel_t = uint32_t;

constexpr static const voxel_t NULL_VOXEL = 0x00000000;
constexpr static const uint32_t NULL_VOXEL_ID = 0x000000;
constexpr static const uint8_t NULL_VOXEL_STATE = 0x00;

constexpr static inline voxel_t make_voxel(uint32_t id, uint8_t state)
{
    voxel_t voxel = 0x00000000;
    voxel |= 0xFF000000 & (state << 24);
    voxel |= 0x00FFFFFF & (id);
    return voxel;
}

constexpr static inline uint32_t get_voxel_id(voxel_t voxel)
{
    return static_cast<uint32_t>((voxel & 0x00FFFFFF));
}

constexpr static inline uint8_t get_voxel_state(voxel_t voxel)
{
    return static_cast<uint8_t>((voxel & 0xFF000000) >> 24);
}

#endif /* SHARED_VOXEL_HH */
