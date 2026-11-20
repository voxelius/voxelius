#include "shared/pch.hh"

#include "shared/component/stasis.hh"

#include "shared/component/transform.hh"
#include "shared/world/world.hh"

void Stasis::fixed_update(World& world)
{
    auto& registry = world.registry();
    auto view = registry.view<Transform>();

    for(auto [entity, transform] : view.each()) {
        if(world.find_chunk(transform.chunk)) {
            registry.remove<Stasis>(entity);
        }
        else {
            registry.emplace_or_replace<Stasis>(entity);
        }
    }
}
