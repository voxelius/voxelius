// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_ATLAS_HH
#define CLIENT_ATLAS_HH
#include <cstddef>
#include <glad/gl.h>
#include <string>
#include <vector>

struct AtlasStrip final {
    std::size_t offset {};
    std::size_t plane {};
};

namespace voxel_atlas
{
void create(int width, int height, std::size_t count);
void destroy(void);
} // namespace voxel_atlas

namespace voxel_atlas
{
std::size_t plane_count(void);
GLuint plane_texture(std::size_t plane_id);
void generate_mipmaps(void);
} // namespace voxel_atlas

namespace voxel_atlas
{
AtlasStrip *find_or_load(const std::vector<std::string> &paths);
AtlasStrip *find(const std::vector<std::string> &paths);
} // namespace voxel_atlas

#endif /* CLIENT_ATLAS_HH */
