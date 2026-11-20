#include "client/pch.hh"

#include "client/game.hh"

#include "core/camera.hh"
#include "core/identifier.hh"
#include "core/utils/angles.hh"

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/physics/physics.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/lighting.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/language.hh"
#include "client/metrics.hh"
#include "client/net/transmit.hh"
#include "client/system/interpolation.hh"
#include "client/system/player_target.hh"
#include "client/system/pmove_client.hh"
#include "client/system/pmove_look.hh"

void client_game::init(void)
{
    // The server owns the light; the client only applies what it receives.
    lighting::set_replica(true);

    gamerules::init();
    server_vars::init();

    interpolation::init();

    metrics::init();

    player_look::init();
    player_move::init();
    player_target::init();
}

void client_game::init_late(void)
{
    // empty
}

void client_game::shutdown(void)
{
    // empty
}

void client_game::update(void)
{
    ZoneScoped;

    interpolation::update();

    player_target::update();

    player_look::update();
    player_move::update();
    pmove::update(globals::window_frametime);

    metrics::update();
}

void client_game::update_late(void)
{
    ZoneScoped;

    player_look::update_late();
}

void client_game::fixed_update(void)
{
    ZoneScoped;

    player_move::fixed_update();
}

void client_game::fixed_update_late(void)
{
    ZoneScoped;

    transmit::fixed_update_late();
}

void client_game::layout(void)
{
    metrics::layout();
}

void client_game::render(void)
{
    ZoneScoped;

    player_target::render();
}
