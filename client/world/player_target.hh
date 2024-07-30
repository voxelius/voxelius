// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_WORLD_PLAYER_TARGET_HH
#define CLIENT_WORLD_PLAYER_TARGET_HH
#include <shared/world/coord.hh>
#include <shared/world/vdef.hh>

namespace player_target
{
extern Voxel voxel;
extern VoxelCoord vvec;
extern VoxelCoord vnormal;
extern const VoxelInfo *info;
} // namespace player_target

namespace player_target
{
void init(void);
void update(void);
void render(void);
} // namespace player_target

#endif /* CLIENT_WORLD_PLAYER_TARGET_HH */
