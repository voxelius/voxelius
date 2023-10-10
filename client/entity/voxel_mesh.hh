// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_ENTITY_VOXEL_MESH_HH
#define CLIENT_ENTITY_VOXEL_MESH_HH
#include <client/mesh.hh>
#include <shared/vdef.hh>

struct VoxelMeshComponent final {
    std::array<Mesh, NUM_VOXEL_DRAW> meshes {};
};

#endif /* CLIENT_ENTITY_VOXEL_MESH_HH */
