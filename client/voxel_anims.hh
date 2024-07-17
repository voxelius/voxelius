// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_VOXEL_ANIMS_HH
#define CLIENT_VOXEL_ANIMS_HH
#include <stdint.h>

namespace voxel_anims
{
extern uint64_t frametime;
extern uint64_t nextframe;
extern uint32_t frame;
} // namespace voxel_anims

namespace voxel_anims
{
void init();
void deinit();
void update();
void construct();
void bind_ssbo();
} // namespace voxel_anims

#endif /* CLIENT_VOXEL_ANIMS_HH */
