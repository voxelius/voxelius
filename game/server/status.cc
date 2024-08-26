// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/server/globals.hh>
#include <game/server/sessions.hh>
#include <game/server/status.hh>
#include <game/shared/protocol.hh>

static void on_status_request(const protocol::StatusRequest &packet)
{
    protocol::StatusResponse response = {};
    response.version = protocol::VERSION;
    response.max_players = sessions::max_players;
    response.num_players = sessions::num_players;

    enet_peer_send(packet.peer, 0, protocol::make_packet(response));
}

void status::init(void)
{
    globals::dispatcher.sink<protocol::StatusRequest>().connect<&on_status_request>();
}