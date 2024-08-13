// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/protocol.hh>
#include <server/globals.hh>
#include <server/status.hh>
#include <server/sessions.hh>

static void on_status_request(const protocol::StatusRequest &packet)
{
    protocol::StatusResponse response = {};
    response.version = protocol::VERSION;
    response.max_players = sessions::max_players;
    response.num_players = sessions::num_players;

    protocol::send_packet(packet.peer, response);
}

void status::init(void)
{
    globals::dispatcher.sink<protocol::StatusRequest>().connect<&on_status_request>();
}
