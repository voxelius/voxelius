// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A8326766_15F1_4715_8A9A_1755D2078B11
#define A8326766_15F1_4715_8A9A_1755D2078B11
#include <stdint.h>

using voxel_t = uint32_t;

constexpr static const voxel_t NULL_VOXEL = 0x00000000;
constexpr static const uint16_t NULL_VOXEL_ID = 0x0000;
constexpr static const uint16_t NULL_VOXEL_STATE = 0x0000;

constexpr static inline voxel_t pack_voxel(uint16_t id, uint16_t state)
{
    voxel_t voxel = 0x0000000;
    voxel |= 0xFFFF0000 & (id << 16);
    voxel |= 0x0000FFFF & (state);
    return voxel;
}

constexpr static inline uint16_t get_voxel_id(voxel_t voxel)
{
    return static_cast<uint16_t>((voxel & 0xFFFF0000) >> 16);
}

constexpr static inline uint16_t get_voxel_state(voxel_t voxel)
{
    return static_cast<uint16_t>(voxel & 0x0000FFFF);
}

#endif /* A8326766_15F1_4715_8A9A_1755D2078B11 */
