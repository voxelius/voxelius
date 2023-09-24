// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef C22100A9_D5BB_4CD9_BDAF_D3F44DE7B084
#define C22100A9_D5BB_4CD9_BDAF_D3F44DE7B084
#include <array>
#include <client/mesh.hh>
#include <shared/vdef.hh>
#include <memory>

struct VoxelMeshComponent final {
    std::array<Mesh, NUM_VOXEL_DRAW> meshes {};
};

namespace voxel_mesher
{
void init();
void deinit();
void update();
} // namespace voxel_mesher

#endif/* C22100A9_D5BB_4CD9_BDAF_D3F44DE7B084 */
