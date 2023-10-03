// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_VOXEL_ANIMS_HH
#define CLIENT_VOXEL_ANIMS_HH
#include <shared/types.hh>

namespace voxel_anims
{
extern uint64_t frametime;
extern uint64_t nextframe;
extern uint32_t frame;

void init();
void deinit();
void update();
void construct();
void bind_ssbo();
} // namespace voxel_anims

#endif/* CLIENT_VOXEL_ANIMS_HH */
