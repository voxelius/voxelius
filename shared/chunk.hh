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
