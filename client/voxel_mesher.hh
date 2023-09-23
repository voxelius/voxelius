// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B75ED6E2_F1B2_419B_A230_47C96F13DF69
#define B75ED6E2_F1B2_419B_A230_47C96F13DF69
#include <client/glxx/drawcmd.hh>
#include <client/glxx/vertex_array.hh>
#include <memory>

struct VoxelMesh final {
    size_t triangles {};
    glxx::DrawCommand cmd {};
    glxx::VertexArray vao {};
    glxx::Buffer vbo {};
};

struct VoxelMeshComponent final {
    std::unique_ptr<VoxelMesh> opaque {nullptr};
    std::unique_ptr<VoxelMesh> alpha {nullptr};
    std::unique_ptr<VoxelMesh> fluid {nullptr};
};

namespace voxel_mesher
{
void deinit();
void update();
} // namespace voxel_mesher

#endif/* B75ED6E2_F1B2_419B_A230_47C96F13DF69 */
