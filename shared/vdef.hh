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
constexpr static const voxel_face_t VOXEL_FACE_NORTH    = 0x0000;
constexpr static const voxel_face_t VOXEL_FACE_SOUTH    = 0x0001;
constexpr static const voxel_face_t VOXEL_FACE_EAST     = 0x0002;
constexpr static const voxel_face_t VOXEL_FACE_WEST     = 0x0003;
constexpr static const voxel_face_t VOXEL_FACE_TOP      = 0x0004;
constexpr static const voxel_face_t VOXEL_FACE_BOTTOM   = 0x0005;
constexpr static const voxel_face_t VOXEL_FACE_CROSS_1  = 0x0006;
constexpr static const voxel_face_t VOXEL_FACE_CROSS_2  = 0x0007;
constexpr static const size_t NUM_VOXEL_FACE = 0x0008;

using voxel_draw_t = unsigned short;
constexpr static const voxel_draw_t VOXEL_DRAW_NODRAW   = 0x0000;
constexpr static const voxel_draw_t VOXEL_DRAW_OPAQUE   = 0x0001;
constexpr static const voxel_draw_t VOXEL_DRAW_ALPHA    = 0x0002;
constexpr static const voxel_draw_t VOXEL_DRAW_FLUID    = 0x0003;

using voxel_shape_t = unsigned short;
constexpr static const voxel_shape_t VOXEL_SHAPE_NONE   = 0x0000;
constexpr static const voxel_shape_t VOXEL_SHAPE_CUBE   = 0x0001;
constexpr static const voxel_shape_t VOXEL_SHAPE_SLAB   = 0x0002;
constexpr static const voxel_shape_t VOXEL_SHAPE_STAIRS = 0x0003;
constexpr static const voxel_shape_t VOXEL_SHAPE_CROSS  = 0x0004;
constexpr static const voxel_shape_t VOXEL_SHAPE_LIQUID = 0x0005;

struct VoxelInfo final {
    struct State final {
        voxel_draw_t draw {VOXEL_DRAW_NODRAW};
        std::array<std::string, NUM_VOXEL_FACE> textures {};
        std::array<unsigned int, NUM_VOXEL_FACE> cache {};
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
voxel_id_t assign(const std::string &name, voxel_id_t hint);
VoxelInfo *find(const std::string &name);
VoxelInfo *find(voxel_id_t id);
vdef_iterator begin();
vdef_iterator end();
} // namespace vdef

#endif/* A3D93D73_6AE3_4604_8299_286223F3A718 */
