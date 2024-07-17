// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <shared/cxmath.hh>
#include <shared/ray_dda.hh>
#include <shared/world.hh>

RayDDA::RayDDA(const glm::dvec3 &start, const glm::dvec3 &direction)
    : start{start}, direction{direction}
{

}

RayDDA::RayDDA(const glm::dvec3 &start, const glm::dvec3 &direction, double max_distance)
    : start{start}, direction{direction}
{
    propagate(max_distance);
}

bool RayDDA::propagate(double max_distance)
{
    lstep.x = glm::abs(1.0 / direction.x);
    lstep.y = glm::abs(1.0 / direction.y);
    lstep.z = glm::abs(1.0 / direction.z);

    // FIXME: why can't we use coord::to_world here?
    vv.x = cxmath::floor<VoxelCoord::value_type>(start.x);
    vv.y = cxmath::floor<VoxelCoord::value_type>(start.y);
    vv.z = cxmath::floor<VoxelCoord::value_type>(start.z);

    for(unsigned int d = 0U; d < 3U; ++d) {
        if(direction[d] < 0.0) {
            lengths[d] = lstep[d] * (start[d] - vv[d]);
            vstep[d] = -1;
        }
        else {
            lengths[d] = lstep[d] * (vv[d] + 1.0 - start[d]);
            vstep[d] = +1;
        }
    }

    cv = coord::to_chunk(vv);
    chunk = world::find_chunk(cv);
    voxel = NULL_VOXEL;

    while(distance < max_distance) {
        bool is_done = true;

        for(unsigned int d = 0U; d < 3U; ++d) {
            const double u = lengths[(d + 1U) % 3U];
            const double v = lengths[(d + 2U) % 3U];
            if(lengths[d] < u && lengths[d] < v) {
                vv[d] += vstep[d];
                distance = lengths[d];
                lengths[d] += lstep[d];
                is_done = false;
            }
        }

        if(!is_done) {
            if(const auto ncv = coord::to_chunk(vv); ncv != cv) {
                chunk = world::find_chunk(ncv);
                cv = ncv;
            }

            if(chunk != nullptr) {
                voxel = voxel_storage::get(chunk->storage, coord::to_local(vv));

                if(voxel != NULL_VOXEL) {
                    wpos = start + direction * distance;
                    return true;
                }
            }

            continue;
        }

        break;
    }

    wpos = start + direction * max_distance;
    return false;
}
