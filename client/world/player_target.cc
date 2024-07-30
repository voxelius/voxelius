// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/world/player_target.hh>
#include <client/globals.hh>
#include <client/vdraw.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/world/ray_dda.hh>

constexpr static float MAX_REACH = 16.0f;

Voxel player_target::voxel = NULL_VOXEL;
VoxelCoord player_target::vvec = VoxelCoord();
VoxelCoord player_target::vnormal = VoxelCoord();
const VoxelInfo *player_target::info = nullptr;

void player_target::init(void)
{
    player_target::voxel = NULL_VOXEL;
    player_target::vvec = VoxelCoord();
    player_target::vnormal = VoxelCoord();
    player_target::info = nullptr;
}

void player_target::update(void)
{
    if(globals::registry.valid(globals::player)) {
        RayDDA ray = {};
        RayDDA::setup(ray, view::position, view::direction);

        do {
            if((player_target::voxel = RayDDA::step(ray)) != NULL_VOXEL) {
                player_target::vvec = ray.vpos;
                player_target::vnormal = ray.vnormal;
                player_target::info = vdef::find(player_target::voxel);
                break;
            }
            
            player_target::vvec = VoxelCoord();
            player_target::vnormal = VoxelCoord();
            player_target::info = nullptr;
        } while(ray.distance < MAX_REACH);
    }
    else {
        player_target::voxel = NULL_VOXEL;
        player_target::vvec = VoxelCoord();
        player_target::vnormal = VoxelCoord();
        player_target::info = nullptr;
    }
}

void player_target::render(void)
{
    if(player_target::voxel != NULL_VOXEL) {
        vdraw::begin_depth();
        vdraw::cube(VoxelCoord::to_world(player_target::vvec), Vec3f(1.0f), 2.0f, Vec4f::black());
    }
}
