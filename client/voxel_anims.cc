// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <shared/util/cxmath.hh>
#include <shared/config.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

unsigned int voxel_anims::fps = 16U;

std::uint64_t voxel_anims::nextframe = 0;
std::uint32_t voxel_anims::frame = 0;

static GLuint uniform_buffer = 0;

void voxel_anims::init(void)
{
    Config::add(globals::client_config, "voxel_anims.fps", voxel_anims::fps);

    voxel_anims::nextframe = 0;
    voxel_anims::frame = 0;

    uniform_buffer = 0;
}

void voxel_anims::deinit(void)
{
    if(uniform_buffer)
        glDeleteBuffers(1, &uniform_buffer);
    voxel_anims::nextframe = 0;
    voxel_anims::frame = 0;
    uniform_buffer = 0;
}

void voxel_anims::update(void)
{
    if(globals::curtime >= voxel_anims::nextframe) {
        voxel_anims::fps = util::clamp(voxel_anims::fps, 2U, 16U);
        voxel_anims::nextframe = globals::curtime + static_cast<std::uint64_t>(1000000.0 / static_cast<float>(voxel_anims::fps));
        voxel_anims::frame += 1U;
    }
}

void voxel_anims::construct(void)
{
    std::vector<GLuint> contents = {};

    for(VoxelInfo *info : vdef::voxels) {
        if(!info->animated) {
            // Skip varied voxel types
            continue;
        }

        VoxelInfoAnimated *info_a = static_cast<VoxelInfoAnimated *>(info);

        for(VoxelTextureAnimated &vtex : info_a->textures) {
            vtex.cached_offset = contents.size();

            for(const std::string &path : vtex.paths) {
                if(const AtlasTexture *atex = voxel_atlas::find(path, true)) {
                    contents.push_back(atex->index);
                    continue;
                }

                spdlog::critical("voxel_anims: {}: {} is not loaded!", info_a->name, path);
                std::terminate();
            }
        }
    }

    GLint64 max_ubo_size = {};
    glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);
    const std::size_t ubo_size = sizeof(GLuint) * contents.size();

    if(ubo_size > max_ubo_size) {
        spdlog::critical("voxel_anims: uniform buffer overrun");
        std::terminate();
    }

    if(!uniform_buffer)
        glGenBuffers(1, &uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(ubo_size), contents.data(), GL_STATIC_DRAW);
}

GLuint voxel_anims::buffer(void)
{
    return uniform_buffer;
}
