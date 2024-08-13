// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <server/game.hh>
#include <server/network.hh>
#include <server/session.hh>
#include <server/status.hh>

void server_game::init(void)
{
    Session::init();
    
    server_network::init();
    
    status::init();
}

void server_game::init_late(void)
{
    Session::init_late();
    
    server_network::init_late();
}

void server_game::deinit(void)
{
    server_network::deinit();

    Session::deinit();
}

void server_game::update(void)
{
    
}

void server_game::update_late(void)
{
    server_network::update_late();
}
