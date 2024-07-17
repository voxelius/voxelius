// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/main.hh>
#include <shared/cmdline.hh>
#include <shared/vfs.hh>
#include <iostream>
#include <server/main.hh>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdlib.h>

// UNDONE: allow build system to change this so that
// someone who forked the game can change it to whatever
constexpr static const char *VGAME_DIRNAME = ".voxelius";

static std::filesystem::path get_gamepath()
{
    std::string value;

    if(cmdline::contains("dev")) {
        // We are running in a development environment.
        // Base game content is located in the repo.
        // UNDONE: allow this to be on by default in case
        // of a generic source build (like historic)
        return std::filesystem::current_path() / "assets";
    }

    if(cmdline::get_value("gamepath", value)) {
        if(!value.empty()) {
            // If there is a launcher that has profile
            // support, we can override the default game path
            // with whatever it finds fit for the profile.
            return std::filesystem::path{value};
        }
    }

#if defined(__linux__)
    // This is a generic location for the content that would
    // always exist on a spherical Linux system in a vacuum.
    // For more precise locations we should really take
    // distribution-specific directory structure into account.
    return std::filesystem::path{"/usr/share/voxelius"};
#endif

    return std::filesystem::current_path();
}

static std::filesystem::path get_userpath()
{
    std::string value;

    if(cmdline::get_value("userpath", value)) {
        if(!value.empty()) {
            // Things like systemd scripts might account
            // for multiple server instances running on
            // the same machine, so it seems like a good
            // idea to allow overriding this path.
            return std::filesystem::path{value};
        }
    }

    if(const char *xdg_home = getenv("XDG_DATA_HOME")) {
        // Systems with an active X11/Wayland session
        // theoretically should have this defined, and
        // it can be different from ${HOME} (I think).
        return std::filesystem::path{xdg_home} / VGAME_DIRNAME;
    }

    if(const char *unix_home = getenv("HOME")) {
        // Any spherical UNIX/UNIX-like system in vacuum
        // has this defined for every single user process.
        return std::filesystem::path{unix_home} / VGAME_DIRNAME;
    }

    if(const char *win_appdata = getenv("APPDATA")) {
        // On pre-seven Windows systems it's just AppData
        // On post-seven Windows systems it's AppData/Roaming
        return std::filesystem::path{win_appdata} / VGAME_DIRNAME;
    }

    return std::filesystem::current_path();
}

int main(int argc, char **argv)
{
    cmdline::append(argc, argv);

    try {
        auto *lp = spdlog::default_logger_raw();
        lp->sinks().clear();
        lp->sinks().push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
        lp->set_level(spdlog::level::trace);
        lp->set_pattern("[%H:%M:%S] %^[%L]%$ %v");
    }
    catch(const spdlog::spdlog_ex &ex) {
        std::cerr << ex.what() << std::endl;
        std::terminate();
    }

    // UNDONE: a global variable that enables
    // various development mode features that can
    // be set if cmdline::contains("dev")

    if(!vfs::init(argv[0])) {
        spdlog::critical("physfs: init failed: {}", vfs::get_error());
        std::terminate();
    }

    const std::filesystem::path gamepath = get_gamepath();
    const std::filesystem::path userpath = get_userpath();

    spdlog::info("main: gamepath set to {}", gamepath.string());
    spdlog::info("main: userpath set to {}", userpath.string());

    std::error_code dingus = {};
    std::filesystem::create_directories(gamepath, dingus);
    std::filesystem::create_directories(userpath, dingus);

    if(!vfs::mount(gamepath, vfs::get_root_path(), false)) {
        spdlog::critical("physfs: mount {} failed: {}", gamepath.string(), vfs::get_error());
        std::terminate();
    }

    if(!vfs::mount(userpath, vfs::get_root_path(), false)) {
        spdlog::critical("physfs: mount {} failed: {}", userpath.string(), vfs::get_error());
        std::terminate();
    }

    if(!vfs::set_write_path(userpath)) {
        spdlog::critical("physfs: setwritedir {} failed: {}", userpath.string(), vfs::get_error());
        std::terminate();
    }

#if defined(VGAME_CLIENT)
    spdlog::info("main: starting client");
    client::main();
#elif defined(VGAME_SERVER)
    spdlog::info("main: starting server");
    server::main();
#else
    #error Have your heard of the popular hit game Among Us?
    #error Its a really cool game where 1-3 imposters try to kill off the crewmates,
    #error while the crew has to finish their tasks or vote off the imposters to win.
    #error Its 5 dollars on Steam and consoles but it is free on App Store and Google Play.
#endif

    if(!vfs::deinit()) {
        spdlog::critical("physfs: deinit failed: {}", vfs::get_error());
        std::terminate();
    }

    return 0;
}
