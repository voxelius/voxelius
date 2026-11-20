#include "client/pch.hh"

#include "client/game.hh"

#include "core/camera.hh"
#include "core/identifier.hh"
#include "core/threading.hh"
#include "core/utils/angles.hh"

#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/physics/physics.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"
#include "shared/world/world_registry.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/language.hh"
#include "client/metrics.hh"
#include "client/net/transmit.hh"
#include "client/system/interpolation.hh"
#include "client/system/player_target.hh"
#include "client/system/pmove_client.hh"
#include "client/system/pmove_look.hh"
#include "client/world/chunk_mesher.hh"

static std::vector<std::shared_ptr<World>> s_worlds;

std::span<const std::shared_ptr<World>> client_game::worlds(void)
{
    return s_worlds;
}

void client_game::open_world(const Identifier& id)
{
    if(globals::world && id == globals::world->id()) {
        return;
    }

    // In-flight meshing and light
    // tasks still hold the old world
    threading::wait();

    chunk_mesher::reset();

    if(globals::world) {
        globals::world->purge();
    }

    lighting::reset();

    for(const auto& world : s_worlds) {
        if(id == world->id()) {
            globals::world = world.get();
            return;
        }
    }

    auto definition = world_registry::find(id);

    if(definition == nullptr) {
        return;
    }

    auto world = std::make_shared<World>(definition);
    s_worlds.push_back(world);
    globals::world = world.get();
}

void client_game::init_world(void)
{
    // Every registered world gets a runtime, so the renderers can connect
    // to all of them once and a world switch only moves globals::world
    s_worlds.clear();

    for(const auto& definition : world_registry::all()) {
        s_worlds.push_back(std::make_shared<World>(definition));
    }

    if(s_worlds.empty()) {
        return;
    }

    auto default_id = Identifier::from_string("builtin:overworld");
    client_game::open_world(default_id);

    if(globals::world == nullptr) {
        globals::world = s_worlds.front().get();
    }
}

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
    pmove::update(globals::world->registry(), globals::window_frametime);

    metrics::update();
}

void client_game::update_late(void)
{
    ZoneScoped;

    player_look::update_late();
}

void client_game::fixed_update(World* world)
{
    ZoneScoped;

    player_move::fixed_update();

    if(world) {
        auto& registry = world->registry();

        Transform::fixed_update(registry);
        Velocity::fixed_update(registry);

        world->fixed_update();
    }
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
