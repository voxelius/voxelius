#include "client/pch.hh"

#include "client/system/interpolation.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/entity/class.hh"
#include "shared/entity/component_map.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"

static void on_transform_construct(entt::registry& registry, entt::entity entity)
{
    const auto& transform = registry.get<Transform>(entity);
    registry.emplace_or_replace<Transform_Intr>(entity, transform);
    registry.emplace_or_replace<Transform_Prev>(entity, transform);
}

static void on_head_construct(entt::registry& registry, entt::entity entity)
{
    const auto& head = registry.get<Head>(entity);
    registry.emplace_or_replace<Head_Intr>(entity, head);
    registry.emplace_or_replace<Head_Prev>(entity, head);
}

static void on_velocity_construct(entt::registry& registry, entt::entity entity)
{
    const auto& velocity = registry.get<Velocity>(entity);
    registry.emplace_or_replace<Velocity_Intr>(entity, velocity);
    registry.emplace_or_replace<Velocity_Prev>(entity, velocity);
}

static void interpolate_components(float alpha)
{
    auto& registry = globals::world->registry();

    for(auto entity : registry.view<EntityClass>()) {
        component_map::interpolate(registry, entity, alpha);
    }
}

void interpolation::init(void)
{
    auto& registry = globals::world->registry();

    auto transform_sink = registry.on_construct<Transform>();
    transform_sink.connect<&on_transform_construct>();

    auto head_sink = registry.on_construct<Head>();
    head_sink.connect<&on_head_construct>();

    auto velocity_sink = registry.on_construct<Velocity>();
    velocity_sink.connect<&on_velocity_construct>();
}

void interpolation::update(void)
{
    auto alpha = static_cast<float>(globals::fixed_accumulator_us) / static_cast<float>(globals::fixed_frametime_us);
    interpolate_components(alpha);
}
