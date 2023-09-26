// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef C9D5143C_9F77_4370_AA09_61AA3A6C2F37
#define C9D5143C_9F77_4370_AA09_61AA3A6C2F37
#include <client/mesh.hh>
#include <shared/vdef.hh>

struct VoxelVertex final {
    vec3f_t position {};
    vec3f_t norm {};
    vec2f_t tex_uv {};
    uint32_t tex_id {};
};

struct VoxelMeshComponent final {
    std::array<Mesh, NUM_VOXEL_DRAW> meshes {};
};

using VoxelMeshBuilder = MeshBuilder<VoxelVertex>;

namespace voxel_mesher
{
void init();
void deinit();
void update();
} // namespace voxel_mesher

#endif/* C9D5143C_9F77_4370_AA09_61AA3A6C2F37 */
