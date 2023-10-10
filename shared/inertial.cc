// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/globals.hh>
#include <shared/inertial.hh>

void inertial::update(double frametime)
{
    const auto &group = globals::registry.group<VelocityComponent>(entt::get<TransformComponent>);

    for(const auto [entity, velocity, transform] : group.each()) {
        // Any collision checks with subsequent velocity updates
        // should be done BEFORE inertial::update is called. Otherwise
        // any collision will have some funky shit going on
        transform.position += velocity.value * frametime;
    }
}
