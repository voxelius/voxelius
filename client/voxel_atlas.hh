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
#ifndef CLIENT_VOXEL_ATLAS_HH
#define CLIENT_VOXEL_ATLAS_HH
#include <client/glxx/texture.hh>
#include <shared/vfs.hh>

struct AtlasTexture final {
    uint32_t texture {};
};

namespace voxel_atlas
{
void create(int width, int height, size_t count);
void generate_mipmap();
void destroy();
const AtlasTexture *load(const vfs::path_t &path);
const AtlasTexture *find(const vfs::path_t &path);
const AtlasTexture *find(const uint32_t texture);
const glxx::Texture2DArray &get();
} // namespace voxel_atlas

#endif /* CLIENT_VOXEL_ATLAS_HH */
