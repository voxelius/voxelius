// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_VOXEL_MESHER_HH
#define CLIENT_VOXEL_MESHER_HH
#include <stddef.h>

namespace voxel_mesher
{
void init();
void deinit();
void update();
size_t get_queue_size();
} // namespace voxel_mesher

#endif /* CLIENT_VOXEL_MESHER_HH */
