#include "shared/pch.hh"

#include "shared/component/stasis.hh"

#include "shared/component/transform.hh"
#include "shared/globals.hh"
#include "shared/world/world.hh"

void Stasis::fixed_update(void)
{
    auto& registry = globals::world->registry();
    auto view = registry.view<Transform>();

    for(auto [entity, transform] : view.each()) {
        if(globals::world->find_chunk(transform.chunk)) {
            registry.remove<Stasis>(entity);
        }
        else {
            registry.emplace_or_replace<Stasis>(entity);
        }
    }
}
