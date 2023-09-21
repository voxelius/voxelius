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

using voxel_type_t = uint8_t;
using voxel_face_t = uint8_t;
using voxel_draw_t = uint8_t;

constexpr static const voxel_t NULL_VOXEL = 0x00000000;
constexpr static const voxel_id_t NULL_VOXEL_ID = 0x0000;
constexpr static const voxel_state_t NULL_VOXEL_STATE = 0x0000;
constexpr static const voxel_facing_t NULL_VOXEL_FACING = 0x00;

constexpr static const voxel_type_t VOXEL_TYPE_CUBE     = 0x00;
constexpr static const voxel_type_t VOXEL_TYPE_SLAB     = 0x01;
constexpr static const voxel_type_t VOXEL_TYPE_STAIRS   = 0x02;
constexpr static const voxel_type_t VOXEL_TYPE_CROSS    = 0x03;
constexpr static const voxel_type_t VOXEL_TYPE_FLUID    = 0x04;
constexpr static const voxel_type_t VOXEL_TYPE_CUSTOM   = 0x05;

constexpr static const voxel_face_t VOXEL_FACE_NORTH    = 0x00;
constexpr static const voxel_face_t VOXEL_FACE_SOUTH    = 0x01;
constexpr static const voxel_face_t VOXEL_FACE_EAST     = 0x02;
constexpr static const voxel_face_t VOXEL_FACE_WEST     = 0x03;
constexpr static const voxel_face_t VOXEL_FACE_TOP      = 0x04;
constexpr static const voxel_face_t VOXEL_FACE_BOTTOM   = 0x05;
constexpr static const voxel_face_t VOXEL_FACE_CROSS1   = 0x06;
constexpr static const voxel_face_t VOXEL_FACE_CROSS2   = 0x07;

constexpr static const voxel_draw_t VOXEL_DRAW_NODRAW   = 0x00;
constexpr static const voxel_draw_t VOXEL_DRAW_OPAQUE   = 0x01;
constexpr static const voxel_draw_t VOXEL_DRAW_ALPHA    = 0x02;
constexpr static const voxel_draw_t VOXEL_DRAW_CHROMA   = 0x03;

class VoxelStateInfo final {
public:
    void add_texture(const std::string &path);
    void set_draw_mode(voxel_draw_t draw_mode);

    const std::string &get_texture(voxel_face_t face, const voxel_coord_t &vvec) const;
    const uint64_t get_texture_hash(voxel_face_t face, const voxel_coord_t &vvec) const;
    const voxel_draw_t get_draw_mode() const;

private:
    std::unordered_map<voxel_face_t, std::vector<std::string>> textures {};
    std::unordered_map<voxel_face_t, std::vector<uint64_t>> texture_hashes {};
    voxel_draw_t draw_mode {VOXEL_DRAW_NODRAW};
};

class VoxelInfo final {

};

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
