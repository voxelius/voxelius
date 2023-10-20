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

        const auto center = transform.position + 0.5 * collision.size;
        const auto pcenter = center + frametime * velocity.linear;

        voxel_pos_t vmod = {};
        vmod.x = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.x));
        vmod.y = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.y));
        vmod.z = cxmath::ceil<voxel_pos_t::value_type>(glm::abs(velocity.linear.z));

        voxel_pos_t vstart = {};
        vstart.x = cxmath::floor<voxel_pos_t::value_type>(pbox.min.x);
        vstart.y = cxmath::floor<voxel_pos_t::value_type>(pbox.min.y);
        vstart.z = cxmath::floor<voxel_pos_t::value_type>(pbox.min.z);

        voxel_pos_t vend = {};
        vend.x = cxmath::floor<voxel_pos_t::value_type>(pbox.max.x);
        vend.y = cxmath::floor<voxel_pos_t::value_type>(pbox.max.y);
        vend.z = cxmath::floor<voxel_pos_t::value_type>(pbox.max.z);

        // PILLOWING: at high velocities, when the object's next sampled position
        // clips inside a voxel, its velocity is se to zero, which produces
        // some kind of a pillow effect, setting the speed along respective
        // axis to zero. The name "pillowing" goes from the fact that it
        // practically stops you mid air as if you were landing on a pillow.
        double pillow_dist = std::numeric_limits<double>::infinity();
        AABB pillow_vbox = {};

        for(auto x = vstart.x; x <= vend.x; ++x)
        for(auto y = vstart.y; y <= vend.y; ++y)
        for(auto z = vstart.z; z <= vend.z; ++z) {
            const voxel_pos_t vpos = voxel_pos_t{x, y, z};
            const voxel_t voxel = world::get_voxel(vpos);

            if(voxel != NULL_VOXEL) {
                AABB vbox = {};
                vbox.min.x = static_cast<double>(vpos.x);
                vbox.min.y = static_cast<double>(vpos.y);
                vbox.min.z = static_cast<double>(vpos.z);
                vbox.max = vbox.min + glm::dvec3{1.0, 1.0, 1.0};

                for(unsigned int d = 0U; d < 3U; ++d) {
                    AABB tbox = {};
                    tbox.min = box.min;
                    tbox.max = box.max;

                    tbox.min[d] = pbox.min[d];
                    tbox.max[d] = pbox.max[d];

                    if(tbox.intersect(vbox)) {
                        if((velocity.linear.y < 0.0) && (d == 1U)) {
                            if(const auto dist = pcenter[d] - vbox.min[d] - 0.5; dist < pillow_dist) {
                                pillow_dist = dist;
                                pillow_vbox = vbox;
                            }
                        }

                        velocity.linear[d] = 0.0;
                    }
                }
            }
        }

        if(pillow_dist < std::numeric_limits<double>::infinity()) {
            if(pbox.min.y < pillow_vbox.max.y) {
                // Snap entity to the ground.
                // UNDONE: set a flag of sorts to true?
                transform.position.y = pillow_vbox.max.y;
            }
        }
    }
}
