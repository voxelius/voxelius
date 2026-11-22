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

    registry.on_construct<Transform>().connect<&on_transform_construct>();
    registry.on_construct<Head>().connect<&on_head_construct>();
    registry.on_construct<Velocity>().connect<&on_velocity_construct>();
}

void interpolation::update(void)
{
    auto alpha = static_cast<float>(globals::fixed_accumulator_us) / static_cast<float>(globals::fixed_frametime_us);

    interpolate_components(alpha);
}

void interpolation::snap(entt::entity entity)
{
    auto& registry = globals::world->registry();

    if(auto transform = registry.try_get<Transform>(entity)) {
        registry.emplace_or_replace<Transform_Prev>(entity, *transform);
        registry.emplace_or_replace<Transform_Intr>(entity, *transform);
    }

    if(auto head = registry.try_get<Head>(entity)) {
        registry.emplace_or_replace<Head_Prev>(entity, *head);
        registry.emplace_or_replace<Head_Intr>(entity, *head);
    }

    if(auto velocity = registry.try_get<Velocity>(entity)) {
        registry.emplace_or_replace<Velocity_Prev>(entity, *velocity);
        registry.emplace_or_replace<Velocity_Intr>(entity, *velocity);
    }
}
