// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_PLAYER_TARGET_HH
#define CLIENT_PLAYER_TARGET_HH
#include <shared/coord.hh>
#include <shared/vdef.hh>

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

#endif /* CLIENT_PLAYER_TARGET_HH */
