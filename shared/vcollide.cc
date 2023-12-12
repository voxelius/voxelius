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
        const auto savevel = velocity.linear;

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

        voxel_pos_t::value_type max_y = std::numeric_limits<voxel_pos_t::value_type>::min();
        voxel_pos_t::value_type min_y = std::numeric_limits<voxel_pos_t::value_type>::max();

        for(auto x = vstart.x; x <= vend.x; ++x)
        for(auto y = vstart.y; y <= vend.y; ++y)
        for(auto z = vstart.z; z <= vend.z; ++z) {
            const auto vpos = voxel_pos_t{x, y, z};
            const auto voxel = world::get_voxel(vpos);

            if(voxel == NULL_VOXEL) {
                // Air does not have collision
                continue;
            }

            AABB vbox = {};
            vbox.min.x = static_cast<double>(vpos.x);
            vbox.min.y = static_cast<double>(vpos.y);
            vbox.min.z = static_cast<double>(vpos.z);
            vbox.max = vbox.min + 1.0;

            // We check horizontal collision first
            constexpr static const std::array<unsigned int, 3> DIMS = {0U, 2U, 1U};

            for(const unsigned int d : DIMS) {
                AABB tbox = {};
                tbox.min = box.min;
                tbox.max = box.max;

                tbox.min[d] = pbox.min[d];
                tbox.max[d] = pbox.max[d];

                if(tbox.intersect(vbox)) {
                    velocity.linear[d] = 0.0;

                    if(d == 1U) {
                        if(vpos[d] >= max_y)
                            max_y = vpos[d];
                        if(vpos[d] <= min_y)
                            min_y = vpos[d];
                    }
                }
            }
        }

        if((savevel.y < 0.0) && (max_y > std::numeric_limits<voxel_pos_t::value_type>::min())) {
            spdlog::trace("SNAP! max_y={}", max_y);
            transform.position.y = max_y + 1.0;
        }

        if((savevel.y > 0.0) && (min_y < std::numeric_limits<voxel_pos_t::value_type>::max())) {
            spdlog::trace("SNAP! min_y={}", min_y);
            transform.position.y = min_y - collision.size.y;
        }
    }
}
