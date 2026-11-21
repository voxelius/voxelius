#include "server/pch.hh"

#include "server/game.hh"

#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/identifier.hh"

#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/entity.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"

#include "server/system/pmove_validator.hh"
#include "server/universe.hh"
#include "server/world/time_sim.hh"
#include "server/world/weather_sim.hh"
#include "server/world/world.hh"

static cvar::status cmd_stop(const cvar::Context&, std::span<const std::string_view>)
{
    std::raise(SIGINT);

    return cvar::status::OK;
}

void server_game::init(void)
{
    lighting::set_replica(false);

    gamerules::init();
    server_vars::init();

    pmove_validator::init();
    time_sim::init();

    auto& stop_binding = cvar::bind_cmd("stop");
    stop_binding.set_help("Stop the server");
    stop_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    stop_binding.connect<&cmd_stop>();
}

void server_game::init_late(void)
{
    auto& default_world = universe::default_world();
    auto& registry = default_world->registry();

    utils::spawn(registry, Identifier::from_string("builtin:test_entity"));

    // Mods and the universe seed are both ready by this point.
    weather_sim::init();
}

void server_game::shutdown(void)
{
    weather_sim::shutdown();
}

void server_game::fixed_update(World* world)
{
    ZoneScoped;

    if(world) {
        auto& registry = world->registry();

        Transform::fixed_update(registry);
        Velocity::fixed_update(registry);

        world->fixed_update();

        pmove::update(registry, globals::fixed_frametime);
    }
}

void server_game::fixed_update_late(void)
{
    ZoneScoped;

    pmove_validator::fixed_update_late();
    weather_sim::fixed_update();
}
