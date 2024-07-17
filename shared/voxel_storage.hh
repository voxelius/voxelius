// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VOXEL_STORAGE_HH
#define SHARED_VOXEL_STORAGE_HH
#include <shared/coord.hh>
#include <shared/voxel.hh>

using VoxelStorageType = unsigned short;
constexpr static VoxelStorageType VOXEL_STORAGE_SINGLE_VOXEL    = 0x0000;
constexpr static VoxelStorageType VOXEL_STORAGE_PALETTE_UINT8   = 0x0001;
constexpr static VoxelStorageType VOXEL_STORAGE_PALETTE_UINT16  = 0x0002;

struct VoxelStorage final {
    VoxelStorageType type {};
    void *voxels {nullptr};
};

namespace voxel_storage
{
void create(VoxelStorage &storage);
void clone(VoxelStorage &storage, const VoxelStorage &parent);
void destroy(VoxelStorage &storage);
} // namespace voxel_storage

namespace voxel_storage
{
Voxel get(const VoxelStorage &storage, const LocalCoord &lv);
Voxel get(const VoxelStorage &storage, std::size_t index);
} // namespace voxel_storage

namespace voxel_storage
{
void set(VoxelStorage &storage, Voxel voxel, const LocalCoord &lv);
void set(VoxelStorage &storage, Voxel voxel, std::size_t index);
} // namespace voxel_storage

namespace voxel_storage
{
void fill(VoxelStorage &storage, Voxel voxel);
} // namespace voxel_storage


#endif /* SHARED_VOXEL_STORAGE_HH */
