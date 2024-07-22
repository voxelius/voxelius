// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VOXEL_ATLAS_HH
#define CLIENT_VOXEL_ATLAS_HH
#include <cstddef>
#include <glad/gl.h>
#include <string>

struct AtlasTexture final {
    std::size_t plane {};
    std::size_t index {};
};

// Plane 0 is always used for animations; The GL spec
// limit of 256 array texture layers is expected to be
// respected by both vdef and voxel_anims subsystems
constexpr static std::size_t ANIMATION_PLANE_ID = 0;

namespace voxel_atlas
{
void init(int width, int height);
void deinit(void);
} // namespace voxel_atlas

namespace voxel_atlas
{
std::size_t plane_count(void);
GLuint plane_texture(std::size_t plane_id);
} // namespace voxel_atlas

namespace voxel_atlas
{
AtlasTexture *find_or_load(const std::string &path, bool animated);
AtlasTexture *find(const std::string &path, bool animated);
} // namespace voxel_atlas

#endif /* CLIENT_VOXEL_ATLAS_HH */
