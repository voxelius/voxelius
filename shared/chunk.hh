// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/const.hh>
#include <shared/voxel.hh>

struct Chunk final {
    using array_type = std::array<voxel_t, CHUNK_VOLUME>;
    entt::entity entity {entt::null};
    array_type voxels {};
};

#endif /* SHARED_CHUNK_HH */
