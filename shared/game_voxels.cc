// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/game_voxels.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

Voxel game_voxels::dirt = NULL_VOXEL;
Voxel game_voxels::grass = NULL_VOXEL;
Voxel game_voxels::slate = NULL_VOXEL;
Voxel game_voxels::stone = NULL_VOXEL;
Voxel game_voxels::vtest = NULL_VOXEL;

void game_voxels::populate(void)
{
    game_voxels::dirt = vdef::create("dirt", VoxelType::Cube).add_default_state().build();
    game_voxels::grass = vdef::create("grass", VoxelType::Cube).add_default_state().build();
    game_voxels::slate = vdef::create("slate", VoxelType::Cube).add_default_state().build();
    game_voxels::stone = vdef::create("stone", VoxelType::Cube).add_default_state().build();
    game_voxels::vtest = vdef::create("vtest", VoxelType::Cube).add_default_state().build();
}
