// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/globals.hh>

void VelocityComponent::update(void)
{
    const auto group = globals::registry.group<VelocityComponent>(entt::get<TransformComponent>);

    for(const auto [entity, velocity, transform] : group.each()) {
        transform.position.local += velocity.linear * globals::frametime;
        transform.angles += velocity.angular * globals::frametime;
    }
}
