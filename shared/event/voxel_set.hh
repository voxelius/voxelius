// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_EVENT_VOXEL_SET_HH
#define SHARED_EVENT_VOXEL_SET_HH
#include <shared/chunks.hh>

struct VoxelSetEvent final {
    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    local_pos_t lpos {};
    voxel_pos_t vpos {};
    size_t index {};
};

#endif /* SHARED_EVENT_VOXEL_SET_HH */
