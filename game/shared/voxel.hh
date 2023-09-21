// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A8326766_15F1_4715_8A9A_1755D2078B11
#define A8326766_15F1_4715_8A9A_1755D2078B11
#include <game/shared/coord.hh>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

using voxel_t = uint32_t;
using voxel_id_t = uint16_t;
using voxel_state_t = uint16_t;
using voxel_facing_t = uint8_t;

constexpr static const voxel_t NULL_VOXEL = 0x00000000;
constexpr static const voxel_id_t NULL_VOXEL_ID = 0x0000;
constexpr static const voxel_state_t NULL_VOXEL_STATE = 0x0000;
constexpr static const voxel_facing_t NULL_VOXEL_FACING = 0x00;

constexpr static inline const voxel_t make_voxel(voxel_id_t id, voxel_state_t state, voxel_facing_t facing)
{
    voxel_t voxel = 0x00000000;
    voxel |= ((id << 16) & 0xFFFF0000);
    voxel |= ((state << 4) & 0x0000FFF0);
    voxel |= ((facing << 0) & 0x0000000F);
    return voxel;
}

constexpr static inline const voxel_id_t get_voxel_id(voxel_t voxel)
{
    return static_cast<voxel_id_t>((voxel & 0xFFFF0000) >> 16);
}

constexpr static inline const voxel_state_t get_voxel_state(voxel_t voxel)
{
    return static_cast<voxel_id_t>((voxel & 0x0000FFF0) >> 4);
}

constexpr static inline const voxel_state_t get_voxel_facing(voxel_t voxel)
{
    return static_cast<voxel_id_t>((voxel & 0x0000000F) >> 0);
}

#endif /* A8326766_15F1_4715_8A9A_1755D2078B11 */
