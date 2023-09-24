// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A3D93D73_6AE3_4604_8299_286223F3A718
#define A3D93D73_6AE3_4604_8299_286223F3A718
#include <array>
#include <shared/voxel.hh>
#include <string>
#include <vector>

using voxel_face_t = unsigned short;
constexpr static const voxel_face_t VOXEL_FACE_NORTH    = 0;
constexpr static const voxel_face_t VOXEL_FACE_SOUTH    = 1;
constexpr static const voxel_face_t VOXEL_FACE_EAST     = 2;
constexpr static const voxel_face_t VOXEL_FACE_WEST     = 3;
constexpr static const voxel_face_t VOXEL_FACE_TOP      = 4;
constexpr static const voxel_face_t VOXEL_FACE_BOTTOM   = 5;
constexpr static const voxel_face_t VOXEL_FACE_CROSS_1  = 6;
constexpr static const voxel_face_t VOXEL_FACE_CROSS_2  = 7;
constexpr static const size_t NUM_VOXEL_FACE = 8;

using voxel_draw_t = unsigned short;
constexpr static const voxel_draw_t VOXEL_DRAW_NODRAW   = 0;
constexpr static const voxel_draw_t VOXEL_DRAW_OPAQUE   = 1;
constexpr static const voxel_draw_t VOXEL_DRAW_ALPHA    = 2;
constexpr static const voxel_draw_t VOXEL_DRAW_FLUID    = 3;
constexpr static const size_t NUM_VOXEL_DRAW = 4;

using voxel_shape_t = unsigned short;
constexpr static const voxel_shape_t VOXEL_SHAPE_NONE   = 0;
constexpr static const voxel_shape_t VOXEL_SHAPE_CUBE   = 1;
constexpr static const voxel_shape_t VOXEL_SHAPE_SLAB   = 2;
constexpr static const voxel_shape_t VOXEL_SHAPE_STAIRS = 3;
constexpr static const voxel_shape_t VOXEL_SHAPE_CROSS  = 4;
constexpr static const voxel_shape_t VOXEL_SHAPE_LIQUID = 5;

struct VoxelInfo final {
    struct State final {
        voxel_draw_t draw {VOXEL_DRAW_NODRAW};
        std::array<std::string, NUM_VOXEL_FACE> textures {};
        std::array<uint32_t, NUM_VOXEL_FACE> cache {};
    };

    voxel_shape_t shape {VOXEL_SHAPE_NONE};
    std::vector<State> states {};
    std::string name {};
};

using vdef_vector = std::vector<VoxelInfo *>;
using vdef_iterator = vdef_vector::iterator;

namespace vdef
{
void purge();
uint16_t assign(const std::string &name, uint16_t hint);
VoxelInfo *find(const std::string &name);
VoxelInfo *find(uint16_t id);
vdef_iterator begin();
vdef_iterator end();
} // namespace vdef

#endif/* A3D93D73_6AE3_4604_8299_286223F3A718 */
