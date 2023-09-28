// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B52C1176_EF80_420D_ABC2_F764832A440C
#define B52C1176_EF80_420D_ABC2_F764832A440C
#include <client/glxx/fwd.hh>
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

#endif/* B52C1176_EF80_420D_ABC2_F764832A440C */
