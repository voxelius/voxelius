// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <glm/geometric.hpp>
#include <shared/ray_dda.hh>
#include <shared/world.hh>

void RayDDA::setup(RayDDA &ray, const EntityPos &start, const glm::fvec3 &direction)
{
    ray.direction = direction;
    ray.start = start;

    ray.delta_dist.x = direction.x ? glm::abs(1.0f / direction.x) : std::numeric_limits<float>::max();
    ray.delta_dist.y = direction.y ? glm::abs(1.0f / direction.y) : std::numeric_limits<float>::max();
    ray.delta_dist.z = direction.z ? glm::abs(1.0f / direction.z) : std::numeric_limits<float>::max();

    ray.distance = 0.0f;
    ray.vpos = coord::to_voxel(ray.start);
    ray.vnormal = VoxelPos(0, 0, 0);

    // Need this for initial direction calculations
    const LocalPos lpos = coord::to_local(start);

    if(direction.x < 0.0f) {
        ray.side_dist.x = ray.delta_dist.x * (ray.start.local.x - lpos.x);
        ray.vstep.x = INT64_C(-1);
    }
    else {
        ray.side_dist.x = ray.delta_dist.x * (lpos.x + 1.0f - ray.start.local.x);
        ray.vstep.x = INT64_C(+1);
    }

    if(direction.y < 0.0f) {
        ray.side_dist.y = ray.delta_dist.y * (ray.start.local.y - lpos.y);
        ray.vstep.y = INT64_C(-1);
    }
    else {
        ray.side_dist.y = ray.delta_dist.y * (lpos.y + 1.0f - ray.start.local.y);
        ray.vstep.y = INT64_C(+1);
    }

    if(direction.z < 0.0f) {
        ray.side_dist.z = ray.delta_dist.z * (ray.start.local.z - lpos.z);
        ray.vstep.z = INT64_C(-1);
    }
    else {
        ray.side_dist.z = ray.delta_dist.z * (lpos.z + 1.0f - ray.start.local.z);
        ray.vstep.z = INT64_C(+1);
    }
}

Voxel RayDDA::step(RayDDA &ray)
{
    if(ray.side_dist.x < ray.side_dist.z) {
        if(ray.side_dist.x < ray.side_dist.y) {
            ray.vnormal = VoxelPos(-ray.vstep.x, 0, 0);
            ray.distance = ray.side_dist.x;
            ray.side_dist.x += ray.delta_dist.x;
            ray.vpos.x += ray.vstep.x;
        }
        else {
            ray.vnormal = VoxelPos(0, -ray.vstep.y, 0);
            ray.distance = ray.side_dist.y;
            ray.side_dist.y += ray.delta_dist.y;
            ray.vpos.y += ray.vstep.y;
        }
    }
    else {
        if(ray.side_dist.z < ray.side_dist.y) {
            ray.vnormal = VoxelPos(0, 0, -ray.vstep.z);
            ray.distance = ray.side_dist.z;
            ray.side_dist.z += ray.delta_dist.z;
            ray.vpos.z += ray.vstep.z;
        }
        else {
            ray.vnormal = VoxelPos(0, -ray.vstep.y, 0);
            ray.distance = ray.side_dist.y;
            ray.side_dist.y += ray.delta_dist.y;
            ray.vpos.y += ray.vstep.y;
        }
    }

    // This is slower than I want it to be
    return world::get_voxel(ray.vpos);
}
