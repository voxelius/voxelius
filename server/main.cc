// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/protocol/protocol.hh>
#include <shared/cmake.hh>
#include <shared/config.hh>
#include <shared/epoch.hh>
#include <server/game.hh>
#include <server/globals.hh>
#include <server/main.hh>
#include <server/globals.hh>
#include <spdlog/spdlog.h>
#include <thread>

constexpr static std::uint64_t TICK_DT = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(protocol::TICKRATE));


void server::main(void)
{
    spdlog::info("server: game version: {}", GAME_VERSION_STRING);
    
    globals::frametime = 0.0f;
    globals::frametime_avg = 0.0f;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    globals::is_running = true;

    server_game::init();

    Config::load(globals::server_config, "server.conf");

    server_game::init_late();

    std::uint64_t last_curtime = globals::curtime;
    
    while(globals::is_running) {
        globals::curtime = epoch::microseconds();
        globals::frametime = static_cast<float>(globals::curtime - last_curtime) / 1000000.0f;
        globals::frametime_avg += globals::frametime;
        globals::frametime_avg *= 0.5f;
        
        last_curtime = globals::curtime;
        
        server_game::update();
        
        std::this_thread::sleep_for(std::chrono::microseconds(TICK_DT));
        
        server_game::update_late();
        
        globals::dispatcher.update();
        
        globals::framecount += 1;
    }
    
    spdlog::info("server: shutdown after {} frames", globals::framecount);
    spdlog::info("server: average framerate: {:.03f} FPS", 1.0f / globals::frametime_avg);
    spdlog::info("server: average frametime: {:.03f} ms", 1000.0f * globals::frametime_avg);
    
    server_game::deinit();
    
    Config::save(globals::server_config, "server.conf");
}
