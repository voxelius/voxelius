// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
