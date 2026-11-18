#include "server/pch.hh"

#include "server/game.hh"

#include "core/identifier.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/entity.hh"
#include "shared/world/lighting.hh"

#include "server/system/pmove_validator.hh"
#include "server/world/time_sim.hh"
#include "server/world/weather_sim.hh"

void server_game::init(void)
{
    lighting::set_replica(false);

    gamerules::init();
    server_vars::init();

    pmove_validator::init();
    time_sim::init();
}

void server_game::init_late(void)
{
    utils::spawn(Identifier::from_string("builtin:test_entity"));

    // Mods and the universe seed are both ready by this point.
    weather_sim::init();
}

void server_game::shutdown(void)
{
    weather_sim::shutdown();
}

void server_game::fixed_update(void)
{
    ZoneScoped;

    pmove::update(globals::fixed_frametime);
}

void server_game::fixed_update_late(void)
{
    ZoneScoped;

    pmove_validator::fixed_update_late();
    weather_sim::fixed_update();
}
