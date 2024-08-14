// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_GAME_VOXELS_HH
#define SHARED_GAME_VOXELS_HH
#include <shared/voxel.hh>

namespace game_voxels
{
extern Voxel dirt;
extern Voxel grass;
extern Voxel slate;
extern Voxel stone;
extern Voxel vtest;
} // namespace game_voxels

namespace game_voxels
{
void populate(void);
} // namespace game_voxels

#endif /* SHARED_GAME_VOXELS_HH */
