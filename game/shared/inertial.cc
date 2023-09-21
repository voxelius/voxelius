// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <game/shared/inertial.hh>
#include <game/shared/transform.hh>
#include <game/shared/velocity.hh>

void inertial::update(World &world, float frametime)
{
    const auto &group = world.registry.group<VelocityComponent>(entt::get<TransformComponent>);

    for(const auto [entity, velocity, transform] : group.each()) {
        // Any collision checks with subsequent velocity updates
        // should be done BEFORE inertial::update is called. Otherwise
        // any collision will have some funky shit going on
        transform.position += velocity.velocity * frametime;
    }
}
