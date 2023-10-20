// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
    vpos.x = cxmath::floor<voxel_pos_t::value_type>(start.x);
    vpos.y = cxmath::floor<voxel_pos_t::value_type>(start.y);
    vpos.z = cxmath::floor<voxel_pos_t::value_type>(start.z);

    for(unsigned int d = 0U; d < 3U; ++d) {
        if(direction[d] < 0.0) {
            lengths[d] = lstep[d] * (start[d] - vpos[d]);
            vstep[d] = -1;
        }
        else {
            lengths[d] = lstep[d] * (vpos[d] + 1.0 - start[d]);
            vstep[d] = +1;
        }
    }

    cpos = coord::to_chunk(vpos);
    chunk = world::find_chunk(cpos);
    voxel = NULL_VOXEL;

    while(distance < max_distance) {
        bool is_done = true;

        for(unsigned int d = 0U; d < 3U; ++d) {
            const double u = lengths[(d + 1U) % 3U];
            const double v = lengths[(d + 2U) % 3U];
            if((lengths[d] < u) && (lengths[d] < v)) {
                vpos[d] += vstep[d];
                distance = lengths[d];
                lengths[d] += lstep[d];
                is_done = false;
            }
        }

        if(!is_done) {
            if(const auto ncpos = coord::to_chunk(vpos); ncpos != cpos) {
                chunk = world::find_chunk(ncpos);
                cpos = ncpos;
            }

            if(chunk != nullptr) {
                voxel = chunk->voxels.at(coord::to_index(coord::to_local(vpos)));
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
