// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/util/cxmath.hh>
#include <shared/ray_dda.hh>
#include <shared/world.hh>

void RayDDA::setup(RayDDA &ray, const WorldPos &start, const Vector3D &direction)
{
    ray.direction = direction;
    ray.start = start;

    ray.delta_dist[0] = direction[0] ? std::fabs(1.0f / direction[0]) : std::numeric_limits<float>::max();
    ray.delta_dist[1] = direction[1] ? std::fabs(1.0f / direction[1]) : std::numeric_limits<float>::max();
    ray.delta_dist[2] = direction[2] ? std::fabs(1.0f / direction[2]) : std::numeric_limits<float>::max();

    ray.distance = 0.0f;
    ray.vpos = WorldPos::to_voxel(ray.start);
    ray.vnormal = VoxelPos(0, 0, 0);

    // Need this for initial direction calculations
    const LocalPos lpos = WorldPos::to_local(start);

    if(direction[0] < 0.0f) {
        ray.side_dist[0] = ray.delta_dist[0] * (ray.start.local[0] - lpos[0]);
        ray.vstep[0] = INT64_C(-1);
    }
    else {
        ray.side_dist[0] = ray.delta_dist[0] * (lpos[0] + 1.0f - ray.start.local[0]);
        ray.vstep[0] = INT64_C(+1);
    }

    if(direction[1] < 0.0f) {
        ray.side_dist[1] = ray.delta_dist[1] * (ray.start.local[1] - lpos[1]);
        ray.vstep[1] = INT64_C(-1);
    }
    else {
        ray.side_dist[1] = ray.delta_dist[1] * (lpos[1] + 1.0f - ray.start.local[1]);
        ray.vstep[1] = INT64_C(+1);
    }

    if(direction[2] < 0.0f) {
        ray.side_dist[2] = ray.delta_dist[2] * (ray.start.local[2] - lpos[2]);
        ray.vstep[2] = INT64_C(-1);
    }
    else {
        ray.side_dist[2] = ray.delta_dist[2] * (lpos[2] + 1.0f - ray.start.local[2]);
        ray.vstep[2] = INT64_C(+1);
    }
}

Voxel RayDDA::step(RayDDA &ray)
{
    if(ray.side_dist[0] < ray.side_dist[2]) {
        if(ray.side_dist[0] < ray.side_dist[1]) {
            ray.vnormal = VoxelPos(-ray.vstep[0], 0, 0);
            ray.distance = ray.side_dist[0];
            ray.side_dist[0] += ray.delta_dist[0];
            ray.vpos[0] += ray.vstep[0];
        }
        else {
            ray.vnormal = VoxelPos(0, -ray.vstep[1], 0);
            ray.distance = ray.side_dist[1];
            ray.side_dist[1] += ray.delta_dist[1];
            ray.vpos[1] += ray.vstep[1];
        }
    }
    else {
        if(ray.side_dist[2] < ray.side_dist[1]) {
            ray.vnormal = VoxelPos(0, 0, -ray.vstep[2]);
            ray.distance = ray.side_dist[2];
            ray.side_dist[2] += ray.delta_dist[2];
            ray.vpos[2] += ray.vstep[2];
        }
        else {
            ray.vnormal = VoxelPos(0, -ray.vstep[1], 0);
            ray.distance = ray.side_dist[1];
            ray.side_dist[1] += ray.delta_dist[1];
            ray.vpos[1] += ray.vstep[1];
        }
    }

    // This is slower than I want it to be
    return world::get_voxel(ray.vpos);
}
