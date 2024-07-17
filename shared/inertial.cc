// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/globals.hh>
#include <shared/inertial.hh>

void inertial::update(double frametime)
{
    const auto &group = globals::registry.group<VelocityComponent>(entt::get<TransformComponent>);

    for(const auto [entity, velocity, transform] : group.each()) {
        transform.angles += velocity.angular * frametime;
        transform.position += velocity.linear * frametime;
    }
}
