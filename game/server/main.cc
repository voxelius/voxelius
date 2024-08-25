// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <common/epoch.hh>
#include <config/cmake.hh>
#include <csignal>
#include <entt/signal/dispatcher.hpp>
#include <game/server/game.hh>
#include <game/server/globals.hh>
#include <game/server/globals.hh>
#include <game/server/main.hh>
#include <mathlib/constexpr.hh>
#include <spdlog/spdlog.h>
#include <thread>

static void on_sigint(int)
{
    spdlog::warn("server: received SIGINT");
    globals::is_running = false;
}

void server::main(void)
{
    spdlog::info("server: game version: {}", GAME_VERSION_STRING);
    
    globals::frametime = 0.0f;
    globals::frametime_avg = 0.0f;
    globals::frametime_us = 0;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    globals::is_running = true;

    std::signal(SIGINT, &on_sigint);

    server_game::init();

    Config::add(globals::server_config, "server.tickrate", globals::tickrate);
    Config::load(globals::server_config, "server.conf");

    globals::tickrate = cxpr::clamp(globals::tickrate, 10U, 300U);
    globals::tickrate_dt = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(globals::tickrate));

    server_game::init_late();

    std::uint64_t last_curtime = globals::curtime;
    
    while(globals::is_running) {
        globals::curtime = epoch::microseconds();
        globals::frametime_us = globals::curtime - last_curtime;
        globals::frametime = static_cast<float>(globals::frametime_us) / 1000000.0f;
        globals::frametime_avg += globals::frametime;
        globals::frametime_avg *= 0.5f;
        
        last_curtime = globals::curtime;
        
        server_game::update();
        server_game::update_late();

        globals::dispatcher.update();
        
        globals::framecount += 1;

        std::this_thread::sleep_for(std::chrono::microseconds(globals::tickrate_dt));
    }

    spdlog::info("server: shutdown after {} frames", globals::framecount);
    spdlog::info("server: average framerate: {:.03f} TPS", 1.0f / globals::frametime_avg);
    spdlog::info("server: average frametime: {:.03f} MSPT", 1000.0f * globals::frametime_avg);

    server_game::deinit();
    
    Config::save(globals::server_config, "server.conf");
}
