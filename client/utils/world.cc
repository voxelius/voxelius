#include "client/pch.hh"

#include "client/utils/world.hh"

#include "shared/globals.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"

static const WorldVisuals s_fallback_visuals {};

const WorldVisuals& utils::world_visuals(void)
{
    if(globals::world == nullptr) {
        return s_fallback_visuals;
    }

    return globals::world->visuals();
}
