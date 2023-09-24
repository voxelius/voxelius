// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef E56B2646_9692_40FB_BBB6_C9868329CF2C
#define E56B2646_9692_40FB_BBB6_C9868329CF2C
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <shared/voxel.hh>

using voxel_array_t = std::array<voxel_t, CHUNK_VOLUME>;

struct Chunk {
    voxel_array_t voxels {};
    entt::entity entity {entt::null};
    short ref_count {};
};

struct ChunkComponent final {
    Chunk *chunk {nullptr};
    chunk_coord_t cvec {};
};

#endif /* E56B2646_9692_40FB_BBB6_C9868329CF2C */
