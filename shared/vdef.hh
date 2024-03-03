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
#ifndef SHARED_VDEF_HH
#define SHARED_VDEF_HH
#include <array>
#include <shared/vfs.hh>
#include <shared/voxel.hh>
#include <unordered_set>
#include <vector>

using voxel_draw_t = unsigned short;
constexpr static const voxel_draw_t VOXEL_DRAW_NODRAW   = 0xFFFF;
constexpr static const voxel_draw_t VOXEL_DRAW_SOLID    = 0x0000;
constexpr static const voxel_draw_t VOXEL_DRAW_CUTOUT   = 0x0001;
constexpr static const voxel_draw_t VOXEL_DRAW_BLEND    = 0x0002;
constexpr static const std::size_t NUM_VOXEL_DRAW = 3;

using voxel_face_t = unsigned short;
constexpr static const voxel_face_t VOXEL_FACE_NORTH    = 0x0000;
constexpr static const voxel_face_t VOXEL_FACE_SOUTH    = 0x0001;
constexpr static const voxel_face_t VOXEL_FACE_EAST     = 0x0002;
constexpr static const voxel_face_t VOXEL_FACE_WEST     = 0x0003;
constexpr static const voxel_face_t VOXEL_FACE_TOP      = 0x0004;
constexpr static const voxel_face_t VOXEL_FACE_BOTTOM   = 0x0005;
constexpr static const std::size_t NUM_VOXEL_FACE = 6;

struct VoxelTexture final {
    std::vector<vfs::path_t> paths {};
    uint16_t offset {};
};

struct VoxelInfo final {
    voxel_draw_t draw {VOXEL_DRAW_NODRAW};
    std::array<VoxelTexture, NUM_VOXEL_FACE> textures {};
    std::string name {};
};

struct VoxelInfoRoot final {
    std::vector<VoxelInfo> vec {};
    std::string name {};
};

namespace vdef
{
extern std::unordered_set<std::string> names;
extern std::unordered_set<vfs::path_t, vfs::path_hasher_t> textures;
extern std::vector<VoxelInfoRoot *> voxels;
bool assign(const std::string &name, uint32_t id);
VoxelInfo *find(uint32_t id, uint8_t state);
VoxelInfo *find(voxel_t voxel);
void purge();
} // namespace vdef

#endif /* SHARED_VDEF_HH */
