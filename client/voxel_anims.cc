// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/globals.hh>
#include <client/glxx/buffer.hh>
#include <client/shaders.hh>
#include <client/voxel_anims.hh>
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

    voxel_anims::frametime = static_cast<uint64_t>(1000000.0f / 10.0f);
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

    for(VoxelInfoRoot *iroot : vdef::voxels) {
        if(iroot != nullptr) {
            for(VoxelInfo &info : iroot->vec) {
                for(VoxelTexture &vtex : info.textures) {
                    vtex.offset = contents.size();
                    for(const vfs::path_t &path : vtex.paths) {
                        const AtlasTexture *atex = atlas::find(path);
                        if(atex == nullptr)
                            std::terminate();
                        contents.push_back(atex->texture);
                    }
                }
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
