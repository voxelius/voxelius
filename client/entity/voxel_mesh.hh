// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_ENTITY_VOXEL_MESH_HH
#define CLIENT_ENTITY_VOXEL_MESH_HH
#include <client/mesh.hh>
#include <shared/vdef.hh>

struct VoxelMeshComponent final {
    std::array<Mesh, NUM_VOXEL_DRAW> meshes {};
};

#endif /* CLIENT_ENTITY_VOXEL_MESH_HH */
