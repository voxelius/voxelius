// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/voxel_atlas.hh>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <shared/util/cxmath.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

struct AtlasPlane final {
    std::unordered_map<std::string, std::size_t> lookup {};
    std::vector<AtlasTexture> textures {};
    std::size_t plane_id {};
    GLuint gl_texture {};
};

static glm::ivec2 atlas_size = {};
static std::vector<AtlasPlane> planes = {};
static std::size_t current_plane = {};
static GLint max_plane_layers = {};

static std::size_t create_atlas_plane(void)
{
    AtlasPlane plane = {};
    plane.plane_id = planes.size();

    glGenTextures(1, &plane.gl_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, plane.gl_texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, atlas_size.x, atlas_size.y, max_plane_layers, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    current_plane = plane.plane_id;
    planes.push_back(std::move(plane));
    return current_plane;
}

static AtlasTexture *plane_lookup(AtlasPlane &plane, const std::string &path)
{
    const auto it = plane.lookup.find(path);
    if(it != plane.lookup.cend())
        return &plane.textures[it->second];
    return nullptr;
}

static AtlasTexture *plane_load_image(AtlasPlane &plane, const std::string &path)
{
    AtlasTexture texture = {};
    texture.index = plane.textures.size();
    texture.plane = plane.plane_id;

    Image image = {};

    if(!Image::load_rgba(image, path, true)) {
        spdlog::warn("voxel_atlas: {}: {}", path, util::physfs_error());
        return nullptr;
    }

    if(image.size != atlas_size) {
        spdlog::warn("voxel_atlas: {}: image size doesn't match atlas size", path);
        Image::unload(image);
        return nullptr;
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, plane.gl_texture);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, texture.index, image.size.x, image.size.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);

    Image::unload(image);

    plane.lookup.emplace(path, texture.index);

    const std::size_t save_index = texture.index;
    plane.textures.push_back(std::move(texture));
    return &plane.textures[save_index];
}

void voxel_atlas::init(int width, int height)
{
    atlas_size.x = 1 << util::log2(width);
    atlas_size.y = 1 << util::log2(height);

    // Clipping this at OpenGL 4.5 limit of 2048 is important due to
    // how voxel quad meshes are packed in memory: each texture index is
    // confined in 11 bits so having bigger atlas planes makes no sense;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_plane_layers);
    max_plane_layers = util::clamp(max_plane_layers, 256, 2048);

    spdlog::debug("voxel_atlas: atlas_size=[{}x{}]", atlas_size.x, atlas_size.y);
    spdlog::debug("voxel_atlas: max_plane_layers={}", max_plane_layers);

    create_atlas_plane();
    create_atlas_plane();
}

void voxel_atlas::deinit(void)
{
    for(AtlasPlane &plane : planes)
        glDeleteTextures(1, &plane.gl_texture);
    max_plane_layers = 0;
    current_plane = 0;
    atlas_size.x = 0;
    atlas_size.y = 0;
    planes.clear();
}

std::size_t voxel_atlas::plane_count(void)
{
    return planes.size();
}

GLuint voxel_atlas::plane_texture(std::size_t plane_id)
{
    if(plane_id < planes.size())
        return planes[plane_id].gl_texture;
    return 0;
}

AtlasTexture *voxel_atlas::find_or_load(const std::string &path, bool animated)
{
    if(AtlasTexture *texture = voxel_atlas::find(path, animated)) {
        // Just return the existing texture
        return texture;
    }

    if(animated) {
        if(planes[ANIMATION_PLANE_ID].textures.size() == max_plane_layers) {
            spdlog::critical("voxel_atlas: animation plane overrun");
            std::terminate();
        }

        return plane_load_image(planes[ANIMATION_PLANE_ID], path);
    }

    if(planes[current_plane].textures.size() == max_plane_layers) {
        // Make sure we always have a free space
        create_atlas_plane();
    }

    return plane_load_image(planes[current_plane], path);
}

AtlasTexture *voxel_atlas::find(const std::string &path, bool animated)
{
    if(!animated) {
        for(std::size_t i = 1; i < planes.size(); ++i) {
            if(AtlasTexture *texture = plane_lookup(planes[i], path))
                return texture;
            continue;
        }

        return nullptr;
    }

    return plane_lookup(planes[ANIMATION_PLANE_ID], path);
}
