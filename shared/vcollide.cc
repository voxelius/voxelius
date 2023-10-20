// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <entt/entity/registry.hpp>
#include <glm/geometric.hpp>
#include <shared/aabb.hh>
#include <shared/entity/collision.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/globals.hh>
#include <shared/vcollide.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

void vcollide::update(double frametime)
{
    const auto group = globals::registry.group(entt::get<CollisionComponent, TransformComponent, VelocityComponent>);

    for(const auto [entity, collision, transform, velocity] : group.each()) {
        AABB box = {};
        box.min = transform.position;
        box.max = transform.position + collision.size;

        AABB pbox = {};
        pbox.min = box.min + frametime * velocity.linear;
        pbox.max = box.max + frametime * velocity.linear;

        voxel_pos_t vmod = {};
        vmod.x = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.x));
        vmod.y = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.y));
        vmod.z = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.z));

        voxel_pos_t vstart = {};
        vstart.x = cxmath::floor<voxel_pos_t::value_type>(box.min.x) - vmod.x;
        vstart.y = cxmath::floor<voxel_pos_t::value_type>(box.min.y) - vmod.y;
        vstart.z = cxmath::floor<voxel_pos_t::value_type>(box.min.z) - vmod.z;

        voxel_pos_t vend = {};
        vend.x = cxmath::floor<voxel_pos_t::value_type>(box.max.x) + vmod.x;
        vend.y = cxmath::floor<voxel_pos_t::value_type>(box.max.y) + vmod.y;
        vend.z = cxmath::floor<voxel_pos_t::value_type>(box.max.z) + vmod.z;

        for(auto x = vstart.x; x <= vend.x; ++x) {
            for(auto y = vstart.y; y <= vend.y; ++y) {
                for(auto z = vstart.z; z <= vend.z; ++z) {
                    const auto vpos = voxel_pos_t{x, y, z};
                    const auto voxel = world::get_voxel(vpos);

                    if(voxel != NULL_VOXEL) {
                        AABB vbox = {};
                        vbox.min = coord::to_world(vpos);
                        vbox.max = vbox.min + 1.0;

                        for(unsigned int d = 0U; d < 3U; ++d) {
                            AABB tbox = box;
                            tbox.min[d] = pbox.min[d];
                            tbox.max[d] = pbox.max[d];

                            if(tbox.intersect(vbox)) {
                                velocity.linear[d] = 0.0;
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
}
