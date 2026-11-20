#include "shared/pch.hh"

#include "shared/component/stasis.hh"

#include "shared/component/transform.hh"
#include "shared/globals.hh"
#include "shared/world/world.hh"

void Stasis::fixed_update(void)
{
    auto view = globals::registry.view<Transform>();

    for(auto [entity, transform] : view.each()) {
        if(globals::world->find_chunk(transform.chunk)) {
            globals::registry.remove<Stasis>(entity);
        }
        else {
            globals::registry.emplace_or_replace<Stasis>(entity);
        }
    }
}
