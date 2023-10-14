// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <glm/geometric.hpp>
#include <shared/raycast.hh>
#include <shared/world.hh>

bool raycast::dda(RayDDA &out, const glm::dvec3 &start, const glm::dvec3 &direction, double max_distance)
{
    Chunk *chunk = nullptr;

    out.start = start;
    out.direction = glm::normalize(direction);

    out.lstep.x = glm::abs(1.0 / out.direction.x);
    out.lstep.y = glm::abs(1.0 / out.direction.y);
    out.lstep.z = glm::abs(1.0 / out.direction.z);

    // FIXME: why exactly can't we use coord::to_voxel() here?
    out.vpos.x = cxmath::floor<voxel_pos_t::value_type>(out.start.x);
    out.vpos.y = cxmath::floor<voxel_pos_t::value_type>(out.start.y);
    out.vpos.z = cxmath::floor<voxel_pos_t::value_type>(out.start.z);

    for(unsigned int d = 0U; d < 3U; ++d) {
        if(out.direction[d] < 0.0) {
            out.lengths[d] = out.lstep[d] * (out.start[d] - out.vpos[d]);
            out.vstep[d] = -1;            
        }
        else {
            out.lengths[d] = out.lstep[d] * (out.vpos[d] + 1.0 - out.start[d]);
            out.vstep[d] = +1;
        }
    }

    out.cpos = coord::to_chunk(out.vpos);
    chunk = world::find_chunk(out.cpos);
    out.voxel = NULL_VOXEL_ID;

    while(out.distance < max_distance) {
        bool is_done = true;

        for(unsigned int d = 0U; d < 3U; ++d) {
            const double u = out.lengths[(d + 1U) % 3U];
            const double v = out.lengths[(d + 2U) % 3U];

            if(out.lengths[d] < u && out.lengths[d] < v) {
                out.vpos[d] += out.vstep[d];
                out.distance = out.lengths[d];
                out.lengths[d] += out.lstep[d];
                is_done = false;
                break;
            }
        }

        if(is_done) {
            // We didn't propagate any
            // further, we are done then.
            break;
        }

        const auto ncpos = coord::to_chunk(out.vpos);
        if(ncpos != out.cpos) {
            chunk = world::find_chunk(ncpos);
            out.cpos = ncpos;
        }

        if(chunk) {
            out.voxel = chunk->voxels.at(coord::to_index(coord::to_local(out.vpos)));
            if(out.voxel != NULL_VOXEL) {
                out.wpos = out.start + out.direction * out.distance;
                return true;
            }
        }
    }

    out.wpos = out.start + out.direction * max_distance;
    return false;
}
