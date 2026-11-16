#include "server/pch.hh"

#include "server/game.hh"

#include "core/identifier.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/globals.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/entity.hh"
#include "shared/world/lighting.hh"

#include "server/system/pmove_validator.hh"

void server_game::init(void)
{
    lighting::set_replica(false);

    gamerules::init(true);

    pmove_validator::init();
}

void server_game::init_late(void)
{
    utils::spawn(Identifier::from_string("builtin:test_entity"));
}

void server_game::shutdown(void)
{
    // empty
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
}
