// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_WORLD_VOXEL_HH
#define SHARED_WORLD_VOXEL_HH
#include <shared/types.hh>

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

#endif /* SHARED_WORLD_VOXEL_HH */
