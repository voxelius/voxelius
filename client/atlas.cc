// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/atlas.hh>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <shared/util/crc64.hh>
#include <shared/util/cxmath.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>

struct AtlasPlane final {
    std::unordered_map<std::size_t, std::size_t> lookup {};
    std::vector<AtlasStrip> strips {};
    std::size_t layer_count_max {};
    std::size_t layer_count {};
    std::size_t plane_id {};
    GLuint gl_texture;
};

static glm::ivec2 atlas_size = {};
static std::size_t atlas_count = {};
static std::vector<AtlasPlane> planes = {};

// Certain animated and varied voxels just double their
// textures (see the "default" texture part in VDefBuilder::build)
// so there could either be six UNIQUE atlas strips or only one
// https://crypto.stackexchange.com/questions/55162/best-way-to-hash-two-values-into-one
static std::size_t vector_hash(const std::vector<std::string> &strings)
{
    std::size_t source = 0;
    for(const std::string &str : strings)
        source += util::crc64(str);
    return util::crc64(&source, sizeof(source));
}

static void plane_setup(AtlasPlane &plane)
{
    glGenTextures(1, &plane.gl_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, plane.gl_texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, atlas_size.x, atlas_size.y, plane.layer_count_max, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static AtlasStrip *plane_lookup(AtlasPlane &plane, std::size_t hash_value)
{
    const auto it = plane.lookup.find(hash_value);
    if(it != plane.lookup.cend())
        return &plane.strips[it->second];
    return nullptr;
}

static AtlasStrip *plane_new_strip(AtlasPlane &plane, const std::vector<std::string> &paths, std::size_t hash_value)
{
    AtlasStrip strip = {};
    strip.offset = plane.layer_count;
    strip.plane = plane.plane_id;
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, plane.gl_texture);
    
    // FIXME: can we maybe figure out a service
    // to NOT load the same image multiple times?
    // I mean, it makes sense if we don't waste system resources.
    for(std::size_t i = 0; i < paths.size(); ++i) {
        Image image = {};
        
        if(!Image::load_rgba(image, paths[i], true)) {
            spdlog::warn("atlas: {}: {}", paths[i], util::physfs_error());
            continue;
        }
        
        if(image.size != atlas_size) {
            spdlog::warn("atlas: {}: size mismatch", paths[i]);
            Image::unload(image);
            continue;
        }
        
        const std::size_t offset = strip.offset + i;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, offset, image.size.x, image.size.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
        Image::unload(image);
    }
    
    plane.layer_count += paths.size();

    const std::size_t index = plane.strips.size();
    plane.lookup.emplace(hash_value, index);
    plane.strips.push_back(std::move(strip));
    return &plane.strips[index];
}

void atlas::create(int width, int height, std::size_t count)
{
    GLint max_plane_layers;

    atlas_size.x = 1 << util::log2(width);
    atlas_size.y = 1 << util::log2(height);

    // Clipping this at OpenGL 4.5 limit of 2048 is important due to
    // how voxel quad meshes are packed in memory: each texture index is
    // confined in 11 bits so having bigger atlas planes makes no sense;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_plane_layers);
    max_plane_layers = util::clamp(max_plane_layers, 256, 2048);

    for(long i = count; i > 0L; i -= max_plane_layers) {
        AtlasPlane plane = {};
        plane.plane_id = planes.size();
        plane.layer_count_max = util::min<std::size_t>(max_plane_layers, i);
        plane.layer_count = 0;

        const std::size_t save_id = plane.plane_id;
        planes.push_back(std::move(plane));
        plane_setup(planes[save_id]);
    }

    spdlog::debug("atlas: count={}", count);
    spdlog::debug("atlas: atlas_size=[{}x{}]", atlas_size.x, atlas_size.y);
    spdlog::debug("atlas: max_plane_layers={}", max_plane_layers);
}

void atlas::destroy(void)
{
    for(const AtlasPlane &plane : planes)
        glDeleteTextures(1, &plane.gl_texture);
    atlas_size.x = 0;
    atlas_size.y = 0;
    planes.clear();
}

std::size_t atlas::plane_count(void)
{
    return planes.size();
}

GLuint atlas::plane_texture(std::size_t plane_id)
{
    if(plane_id < planes.size())
        return planes[plane_id].gl_texture;
    return 0;
}

AtlasStrip *atlas::find_or_load(const std::vector<std::string> &paths)
{
    const std::size_t hash_value = vector_hash(paths);

    for(AtlasPlane &plane : planes) {
        if(AtlasStrip *strip = plane_lookup(plane, hash_value))
            return strip;
        continue;
    }

    for(AtlasPlane &plane : planes) {
        if((plane.layer_count + paths.size()) <= plane.layer_count_max)
            return plane_new_strip(plane, paths, hash_value);
        continue;
    }

    return nullptr;
}

AtlasStrip *atlas::find(const std::vector<std::string> &paths)
{
    const std::size_t hash_value = vector_hash(paths);

    for(AtlasPlane &plane : planes) {
        if(AtlasStrip *strip = plane_lookup(plane, hash_value))
            return strip;
        continue;
    }

    return nullptr;
}
