// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/voxel_anims.hh>
#include <shared/config.hh>
#include <shared/constexpr.hh>

static unsigned int base_framerate = 16U;
std::uint64_t voxel_anims::nextframe = 0U;
std::uint32_t voxel_anims::frame = 0U;

void voxel_anims::init(void)
{
    Config::add(globals::client_config, "voxel_anims.base_framerate", base_framerate);
    voxel_anims::nextframe = 0U;
    voxel_anims::frame = 0U;
}

void voxel_anims::update(void)
{
    if(globals::curtime >= voxel_anims::nextframe) {
        base_framerate = cxpr::clamp(base_framerate, 1U, 16U);
        voxel_anims::nextframe = globals::curtime + static_cast<std::uint64_t>(1000000.0 / static_cast<float>(base_framerate));
        voxel_anims::frame += 1U;
    }
}
