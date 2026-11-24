#include "server/pch.hh"

#include "server/api/libraries.hh"

#include "shared/api/biomes_library.hh"
#include "shared/api/blocks_library.hh"
#include "shared/api/entities_library.hh"
#include "shared/api/fluids_library.hh"
#include "shared/api/precipitation_library.hh"
#include "shared/api/tints_library.hh"
#include "shared/mod_loader.hh"

#include "server/api/core_library.hh"
#include "server/api/weather_library.hh"
#include "server/api/world_library.hh"
#include "server/api/worlds_library.hh"

void api_server::register_libraries(void)
{
    mod_loader::add_library<&api_server::open_core>();
    mod_loader::add_library<&api_shared::open_tints>();
    mod_loader::add_library<&api_shared::open_fluids>();
    mod_loader::add_library<&api_shared::open_blocks>();
    mod_loader::add_library<&api_shared::open_biomes>();
    mod_loader::add_library<&api_shared::open_precipitation>();
    mod_loader::add_library<&api_server::open_weather>();
    mod_loader::add_library<&api_shared::open_entities>();
    mod_loader::add_library<&api_server::open_world>();
    mod_loader::add_library<&api_server::open_worlds>();
}
