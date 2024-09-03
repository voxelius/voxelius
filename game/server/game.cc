// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <common/epoch.hh>
#include <entt/entity/registry.hpp>
#include <game/server/game.hh>
#include <game/server/globals.hh>
#include <game/server/receive.hh>
#include <game/server/sessions.hh>
#include <game/server/status.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/game_voxels.hh>
#include <game/shared/protocol.hh>
#include <game/shared/splash.hh>
#include <game/shared/vgen.hh>
#include <game/shared/world.hh>
#include <mathlib/constexpr.hh>
#include <spdlog/spdlog.h>

static unsigned int listen_port = protocol::PORT;
static unsigned int status_peers = 4U;

void server_game::init(void)
{
    Config::add(globals::server_config, "game.listen_port", listen_port);
    Config::add(globals::server_config, "game.status_peers", status_peers);

    sessions::init();

    splash::init("texts/motds.txt");
    status::init();

    server_recieve::init();

    world::init();

    //vgen::init(epoch::microseconds());
    vgen::init(UINT64_C(42));
}

void server_game::init_late(void)
{
    sessions::init_late();

    listen_port = cxpr::clamp<unsigned int>(listen_port, 1024U, UINT16_MAX);
    status_peers = cxpr::clamp<unsigned int>(status_peers, 2U, 16U);

    ENetAddress address = {};
    address.host = ENET_HOST_ANY;
    address.port = listen_port;

    globals::server_host = enet_host_create(&address, sessions::max_players + status_peers, 1, 0, 0);

    if(!globals::server_host) {
        spdlog::critical("game: unable to setup an ENet host");
        std::terminate();
    }

    spdlog::info("game: host: {} player + {} status peers", sessions::max_players, status_peers);
    spdlog::info("game: host: listening on UDP port {}", address.port);

    game_voxels::populate();

    constexpr int WSIZE = 16;
    for(int x = -WSIZE; x < WSIZE; x += 1) {
        for(int z = -WSIZE; z < WSIZE; z += 1) {
            spdlog::info("generating {} {}", x, z);
            vgen::generate_overworld(x, z);
        }
    }
}

void server_game::deinit(void)
{
    protocol::send_disconnect(nullptr, globals::server_host, "protocol.server_shutdown");

    sessions::deinit();

    enet_host_flush(globals::server_host);
    enet_host_service(globals::server_host, nullptr, 500);
    enet_host_destroy(globals::server_host);
}

void server_game::update(void)
{
#if 0
    const auto group = globals::registry.group<PlayerComponent, HeadComponent, TransformComponent, VelocityComponent>();

    for(const auto [entity, head, transform, velocity] : group.each()) {
        spdlog::info("player: {} {} {} / {} {} {}",
            transform.position.chunk[0], transform.position.chunk[1], transform.position.chunk[2],
            transform.position.local[0], transform.position.local[1], transform.position.local[2]);
    }
#endif
}

void server_game::update_late(void)
{    
    ENetEvent event = {};

    while(enet_host_service(globals::server_host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            sessions::destroy(sessions::find(event.peer));
            continue;
        }

        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            protocol::receive(event.packet, event.peer);
            enet_packet_destroy(event.packet);
            continue;
        }
    }
}
