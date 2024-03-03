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
#ifndef SHARED_EVENT_CHUNK_UPDATE_HH
#define SHARED_EVENT_CHUNK_UPDATE_HH
#include <shared/chunk.hh>
#include <shared/coord.hh>

struct ChunkUpdateEvent final {
    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    local_pos_t lpos {};
    voxel_pos_t vpos {};
    size_t index {};
};

#endif /* SHARED_EVENT_CHUNK_UPDATE_HH */
