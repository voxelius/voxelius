// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/protocol/protocol.hh>
#include <shared/config.hh>
#include <server/globals.hh>
#include <server/network.hh>
#include <server/session.hh>

unsigned int server_network::listen_port = protocol::DEFAULT_PORT;
unsigned int server_network::incoming_bandwidth = 0U;
unsigned int server_network::outgoing_bandwidth = 0U;

void server_network::init(void)
{
    Config::add(globals::server_config, "network.listen_port", server_network::listen_port);
    Config::add(globals::server_config, "network.incoming_bandwidth", server_network::incoming_bandwidth);
    Config::add(globals::server_config, "network.outgoing_bandwidth", server_network::outgoing_bandwidth);
}

void server_network::init_late(void)
{
    server_network::listen_port = cxpr::clamp<unsigned int>(server_network::listen_port, 1024U, UINT16_MAX);

    // NOTE FOR MYSELF - YOU STOPPED WRITING CODE HERE BEFORE GOING TO TAKE A SHIT AND GO TO WORK

}

void server_network::deinit(void)
{

}

void server_network::update_late(void)
{

}

void server_network::kick_all(const std::string &reason)
{

}

void server_network::kick(Session *session, const std::string &reason)
{

}
