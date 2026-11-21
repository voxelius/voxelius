#include "client/pch.hh"

#include "client/api/libraries.hh"

#include "shared/api/biomes_library.hh"
#include "shared/api/blocks_library.hh"
#include "shared/api/entities_library.hh"
#include "shared/api/fluids_library.hh"
#include "shared/api/tints_library.hh"
#include "shared/api/weather_library.hh"
#include "shared/api/world_library.hh"
#include "shared/mod_loader.hh"

#include "client/api/core_library.hh"

void api_client::register_libraries(void)
{
    mod_loader::add_library<&api_client::open_core>();
    mod_loader::add_library<&api_shared::open_tints>();
    mod_loader::add_library<&api_shared::open_fluids>();
    mod_loader::add_library<&api_shared::open_blocks>();
    mod_loader::add_library<&api_shared::open_biomes>();
    mod_loader::add_library<&api_shared::open_weather>();
    mod_loader::add_library<&api_shared::open_entities>();
    mod_loader::add_library<&api_shared::open_world>();
}
