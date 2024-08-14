// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/config.hh>
#include <shared/game_voxels.hh>
#include <shared/protocol.hh>
#include <server/game.hh>
#include <server/globals.hh>
#include <server/sessions.hh>
#include <server/status.hh>
#include <spdlog/spdlog.h>

static unsigned int listen_port = protocol::PORT;
static unsigned int status_peers = 4U;

void server_game::init(void)
{
    Config::add(globals::server_config, "game.listen_port", listen_port);
    Config::add(globals::server_config, "game.status_peers", status_peers);

    sessions::init();

    status::init();
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
}

void server_game::deinit(void)
{
    sessions::deinit();

    enet_host_destroy(globals::server_host);
}

void server_game::update(void)
{
    
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
            protocol::handle_packet(event.packet, event.peer);
            enet_packet_destroy(event.packet);
            continue;
        }
    }
}
