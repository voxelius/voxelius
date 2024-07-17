// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/glxx/buffer.hh>
#include <client/shaders.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <shared/vdef.hh>
#include <vector>

constexpr static const char *voxel_anims_vert = R"glsl(
    layout(std430, binding = 32) buffer _voxel_anims {
        uint voxel_anims[];
    };
)glsl";

static glxx::Buffer ssbo = {};

uint64_t voxel_anims::frametime = 0;
uint64_t voxel_anims::nextframe = 0;
uint32_t voxel_anims::frame = 0;

void voxel_anims::init()
{
    // This module is for vertex shaders only because fetching
    // animation texture IDs in a fragment shader is a war crime
    shaders::add("voxel_anims", GL_VERTEX_SHADER, voxel_anims_vert);

    voxel_anims::frametime = static_cast<uint64_t>(1000000.0 / 10.0);
    voxel_anims::nextframe = 0;
    voxel_anims::frame = 0;
}

void voxel_anims::deinit()
{
    ssbo.destroy();
}

void voxel_anims::update()
{
    if(globals::curtime >= voxel_anims::nextframe) {
        voxel_anims::nextframe = globals::curtime + voxel_anims::frametime;
        voxel_anims::frame += 1;
    }
}

void voxel_anims::construct()
{
    std::vector<uint32_t> contents = {};

    for(VoxelInfo &info : vdef::voxels) {
        for(VoxelTexture &vtex : info.textures) {
            vtex.cached_offset = contents.size();

            for(const vfs::path_t &path : vtex.paths) {
                const AtlasTexture *atex = voxel_atlas::find(path);

                if(atex == nullptr) {
                    spdlog::critical("voxel_anims: atlas texture not found: {}", path.string());
                    std::terminate();
                }

                contents.push_back(atex->texture);
            }
        }
    }

    ssbo.create();
    ssbo.storage(sizeof(uint32_t) * contents.size(), contents.data(), 0);
}

void voxel_anims::bind_ssbo()
{
    ssbo.bind_base(GL_SHADER_STORAGE_BUFFER, 32);
}
