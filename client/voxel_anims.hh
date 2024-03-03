// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
