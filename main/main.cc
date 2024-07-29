// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/main.hh>
#include <cstdlib>
#include <filesystem>
#include <server/main.hh>
#include <shared/util/physfs.hh>
#include <shared/cmdline.hh>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

static std::filesystem::path get_gamepath(void)
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
            return std::filesystem::path(value);
        }
    }

#if defined(__linux__)
    // This is a generic location for the content that would
    // always exist on a spherical Linux system in a vacuum.
    // For more precise locations we should really take
    // distribution-specific directory structure into account.
    return std::filesystem::path("/usr/share/voxelius");
#endif

    return std::filesystem::current_path();
}

static std::filesystem::path get_userpath(void)
{
    std::string value;

    if(cmdline::get_value("userpath", value)) {
        if(!value.empty()) {
            // Things like systemd scripts might account
            // for multiple server instances running on
            // the same machine, so it seems like a good
            // idea to allow overriding this path.
            return std::filesystem::path(value);
        }
    }

    if(const char *win_appdata = std::getenv("APPDATA")) {
        // On pre-seven Windows systems it's just AppData
        // On post-seven Windows systems it's AppData/Roaming
        return std::filesystem::path(win_appdata) / "voxelius";
    }

    if(const char *xdg_home = std::getenv("XDG_DATA_HOME")) {
        // Systems with an active X11/Wayland session
        // theoretically should have this defined, and
        // it can be different from ${HOME} (I think).
        return std::filesystem::path(xdg_home) / ".voxelius";
    }

    if(const char *unix_home = std::getenv("HOME")) {
        // Any spherical UNIX/UNIX-like system in vacuum
        // has this defined for every single user process.
        return std::filesystem::path(unix_home) / ".voxelius";
    }

    return std::filesystem::current_path();
}

int main(int argc, char **argv)
{
    cmdline::append(argc, argv);

    auto *logger = spdlog::default_logger_raw();
    logger->sinks().clear();
    logger->sinks().push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%H:%M:%S] %^[%L]%$ %v");

    if(!PHYSFS_init(argv[0])) {
        spdlog::critical("physfs: init failed: {}", util::physfs_error());
        std::terminate();
    }

    const auto gamepath = get_gamepath();
    const auto userpath = get_userpath();

    spdlog::info("main: set gamepath to {}", gamepath.string());
    spdlog::info("main: set userpath to {}", userpath.string());

    std::error_code dingus = {};
    std::filesystem::create_directories(gamepath, dingus);
    std::filesystem::create_directories(userpath, dingus);

    if(!PHYSFS_mount(gamepath.string().c_str(), nullptr, false)) {
        spdlog::critical("physfs: mount {} failed: {}", gamepath.string(), util::physfs_error());
        std::terminate();
    }

    if(!PHYSFS_mount(userpath.string().c_str(), nullptr, false)) {
        spdlog::critical("physfs: mount {} failed: {}", userpath.string(), util::physfs_error());
        std::terminate();
    }

    if(!PHYSFS_setWriteDir(userpath.string().c_str())) {
        spdlog::critical("physfs: setwritedir {} failed: {}", userpath.string(), util::physfs_error());
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

    if(!PHYSFS_deinit()) {
        spdlog::critical("physfs: deinit failed: {}", util::physfs_error());
        std::terminate();
    }

    return 0;
}
