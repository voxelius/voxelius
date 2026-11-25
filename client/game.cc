#include "client/pch.hh"

#include "client/game.hh"

#include "core/camera.hh"
#include "core/threading.hh"
#include "core/utils/angles.hh"

#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/physics/physics.hh"
#include "shared/system/pmove.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"
#include "shared/world/world_definition.hh"

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
#include "client/world/columns.hh"

static std::unique_ptr<World> s_world;

void client_game::open_world(const packet::World_Info& packet)
{
    assert(globals::world);

    WorldDefinition definition {};
    packet.to_definition(definition);

    // Queued meshes are dropped and only the few running workers are joined: the old
    // world cannot be freed under a task, and the new one must not wait for the whole queue
    threading::cancel_all();

    chunk_mesher::reset();

    s_world->purge();

    globals::player = entt::null;

    lighting::reset(s_world.get());

    s_world->set_definition(std::make_shared<WorldDefinition>(std::move(definition)));
    s_world->set_weather_seed(packet.weather_seed);
    s_world->weather_tick = 0;
    s_world->daylight_tick = constant::DAY_LENGTH_TICKS / 4;
}

void client_game::init_world(void)
{
    auto definition = std::make_shared<WorldDefinition>();

    s_world = std::make_unique<World>(std::move(definition));
    s_world->connect_column_lookup<&columns::find>();

    globals::world = s_world.get();
}

void client_game::init(void)
{
    // The server owns the light; the client only applies what it receives
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
    threading::wait();

    if(globals::world) {
        globals::world->purge();
    }
}

void client_game::fixed_update(World* world)
{
    ZoneScoped;

    player_move::fixed_update();

    if(world) {
        if(gamerules::weather.value()) {
            world->weather_tick += 1;
        }

        auto& registry = world->registry();

        Transform::fixed_update(registry);
        Velocity::fixed_update(registry);
    }
}

void client_game::fixed_update_late(void)
{
    ZoneScoped;

    transmit::fixed_update_late();
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

void client_game::render(void)
{
    ZoneScoped;

    player_target::render();
}

void client_game::layout(void)
{
    metrics::layout();
}
