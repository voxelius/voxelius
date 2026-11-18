#include "server/pch.hh"

#include "core/core.hh"
#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/res/resource.hh"
#include "core/threading.hh"
#include "core/utils/epoch.hh"
#include "core/version.hh"

#include "shared/coord.hh"
#include "shared/cvar/commands.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/game.hh"
#include "shared/splash.hh"

#include "server/constant.hh"
#include "server/game.hh"
#include "server/globals.hh"
#include "server/net/host.hh"
#include "server/net/interest.hh"
#include "server/net/invites.hh"
#include "server/net/operators.hh"
#include "server/net/receive.hh"
#include "server/net/replication.hh"
#include "server/net/sessions.hh"
#include "server/net/status.hh"
#include "server/net/transmit.hh"
#include "server/net/whitelist.hh"
#include "server/system/collector.hh"
#include "server/universe.hh"
#include "server/world/chunk_loader.hh"
#include "server/world/chunk_unloader.hh"
#include "server/world/climate.hh"
#include "server/world/worldgen.hh"

static std::atomic_bool s_is_running;

static void signal_handler(int)
{
    msg::log_info("received termination signal");

    s_is_running.store(false);
}

static void zoned_fixed_update(void)
{
    ZoneScopedN("server::fixed_update");

    shared_game::fixed_update();
    server_game::fixed_update();
}

static void zoned_fixed_update_late(void)
{
    ZoneScopedN("server::fixed_update_late");

    host::fixed_update_late();

    shared_game::fixed_update_late();
    server_game::fixed_update_late();

    interest::fixed_update_late();

    replication::fixed_update_late();

    collector::fixed_update_late();

    chunk_loader::fixed_update();
    chunk_unloader::fixed_update_late();

    receive::fixed_update_late();

    transmit::fixed_update_late();

    threading::update();
}

static void wrapped_main(int argc, char** argv)
{
    core::setup(argc, argv);

    tracy::SetThreadName("Main");
    TracySetProgramName("Voxelius Server");
    TracyAppInfo(version::full.data(), version::full.size());
    TracyPlotConfig("Frametime ms", tracy::PlotFormatType::Number, false, true, 0);

    msg::log_info("engine version: {}", version::full);

    std::signal(SIGINT, &signal_handler);
    std::signal(SIGTERM, &signal_handler);

    host::init();

    shared_game::init();
    server_game::init();

    collector::init();

    interest::init();

    whitelist::init();
    operators::init();
    invites::init();
    sessions::init();
    replication::init();
    status::init();
    receive::init();
    transmit::init();

    // The universe seed has to be loaded before worldgen
    // derives its noise and feature seeds from it
    universe::init();

    worldgen::init();
    chunk_loader::init();
    chunk_unloader::init();

    splash::init(SPLASH_SERVER);

    cvar::register_commands_core();

    cvar::exec_from("server.conf");

    replication::init_late();

    host::init_late();

    universe::init_late();

    shared_game::init_late();
    server_game::init_late();

    sessions::init_late();

    climate::rebuild();

#ifndef NDEBUG
    msg::log_warning("debug build");
#endif

    s_is_running.store(true);

    globals::curtime_us = utils::epoch_microseconds();

    globals::fixed_framecount = 0;
    globals::fixed_frametime_us = constant::FIXED_FRAMETIME_US;
    globals::fixed_frametime = static_cast<float>(globals::fixed_frametime_us) / 1000000.0f;
    globals::fixed_frametime_avg = 0.0f;

    auto last_curtime_us = globals::curtime_us;
    auto actual_frametime_us = globals::fixed_frametime_us;
    auto actual_frametime = 0.0f;
    auto next_tick_us = globals::curtime_us;

    while(s_is_running.load()) {
        globals::curtime_us = utils::epoch_microseconds();

        next_tick_us += constant::FIXED_FRAMETIME_US;

        if(globals::curtime_us < next_tick_us) {
            std::this_thread::sleep_for(std::chrono::microseconds(next_tick_us - globals::curtime_us));
        }
        else {
            next_tick_us = globals::curtime_us;
        }

        actual_frametime_us = globals::curtime_us - last_curtime_us;
        actual_frametime = static_cast<float>(actual_frametime_us) / 1000000.0f;
        globals::fixed_frametime_avg += actual_frametime;
        globals::fixed_frametime_avg *= 0.5f;

        last_curtime_us = globals::curtime_us;

        globals::current_tick += 1;

        if(gamerules::daylight_cycle.value()) {
            globals::daylight_tick += 1;
        }

        zoned_fixed_update();

        zoned_fixed_update_late();

        FrameMark;

        globals::fixed_framecount += 1;

        globals::dispatcher.update();

        res::soft_purge();

        TracyPlot("Fixed frametime ms", 1000.0f * globals::fixed_frametime);
        TracyPlot("Fixed framerate", static_cast<int64_t>(1.0e6f / globals::fixed_frametime));
    }

    msg::log_info("shutdown after {} frames", globals::fixed_framecount);
    msg::log_info("avg tickrate: {:.03f} TPS ({:.03f} MSPT)", 1.0f / globals::fixed_frametime_avg, 1000.0f * globals::fixed_frametime_avg);

    worldgen::shutdown();

    chunk_loader::shutdown();

    sessions::shutdown();
    invites::shutdown();
    whitelist::shutdown();
    operators::shutdown();

    server_game::shutdown();
    shared_game::shutdown();

    // The world config is written last: every subsystem that keeps state in it
    // has already put its own there.
    universe::shutdown();

    host::shutdown();

    // Workers keep thread-local scratch alive until the pool is destroyed;
    // stop it first so that a zombie warning in hard_purge means a real leak
    threading::shutdown();

    res::hard_purge();

    cvar::save_values("server.conf");

    core::teardown();
}

int main(int argc, char** argv)
{
    msg::add_sink<&msg::detail::stderr_sink>();

    try {
        wrapped_main(argc, argv);
        return EXIT_SUCCESS;
    }
    catch(const std::exception& ex) {
        msg::log_error(std::string_view(ex.what()));
        return EXIT_FAILURE;
    }
    catch(const vx::detail::Exception& ex) {
        msg::log_error(ex.what_standard(), ex.location());
        return EXIT_FAILURE;
    }
    catch(...) {
        msg::log_error(std::string_view("unknown exception"));
        return EXIT_FAILURE;
    }
}
