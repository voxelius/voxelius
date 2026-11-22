#include "core/pch.hh"

#include "core/core.hh"

#include "core/cmdline.hh"
#include "core/exception.hh"
#include "core/res/image.hh"
#include "core/threading.hh"
#include "core/utils/physfs.hh"

constexpr static const char* MOUNT_DATA = "data";
constexpr static const char* MOUNT_USER = nullptr;

static std::filesystem::path s_gamepath;
static std::filesystem::path s_userpath;
static std::filesystem::path s_modspath;

const std::filesystem::path& core::gamepath(void)
{
    return s_gamepath;
}

const std::filesystem::path& core::userpath(void)
{
    return s_userpath;
}

const std::filesystem::path& core::modspath(void)
{
    return s_modspath;
}

void core::setup(int argc, char** argv)
{
    cmdline::create(argc, argv);

    auto physfs_init_ok = PHYSFS_init(argv[0]);
    vx::throw_if_not_fmt(physfs_init_ok, "failed to initialize physfs: {}", utils::physfs_error());

    auto enet_init_fail = static_cast<bool>(enet_initialize());
    vx::throw_if<vx::runtime_error>(enet_init_fail, "failed to initialize enet");

    s_gamepath = std::filesystem::absolute(cmdline::value_or("gamepath", "data"));

    if(auto value = cmdline::value_or_cstr("user", nullptr)) {
        // If there is a third-party launcher that supports
        // multiple profiles, it might throw in a custom user
        // directory with different user-specific configuration files
        s_userpath = std::filesystem::absolute(value);
    }
    else if(auto windows_appdata = std::getenv("APPDATA")) {
        // We appear to run on a system/environment that is itself
        // a Windows or pretends to be Windows; regardless, persistent
        // application data for the game is stored in %APPDATA% in this case
        s_userpath = std::filesystem::path(windows_appdata) / "Voxelius";
    }
    else if(auto xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        // We appear to run on a system/environment that complies
        // or tries to comply with freedesktop spec; by default XDG_DATA_HOME
        // should resolve to ${HOME}/.local/share so it's a good place for userpath
        s_userpath = std::filesystem::path(xdg_data_home) / "voxelius";
    }
    else if(auto unix_home = std::getenv("HOME")) {
        // If anything else fails, as far as I'm concerned, every UNIX system
        // defines an environment variable for user's home directory; we can
        // assume default location from freedesktop spec and put game data there
        s_userpath = std::filesystem::path(unix_home) / ".local/share/voxelius";
    }
    else {
        // Give up and save stuff into cwd
        s_userpath = std::filesystem::current_path();
    }

    s_modspath = s_userpath / "mods";

    msg::log_debug("gamepath set to {}", s_gamepath.string());
    msg::log_debug("userpath set to {}", s_userpath.string());
    msg::log_debug("modspath set to {}", s_modspath.string());

    std::filesystem::create_directories(s_userpath);
    std::filesystem::create_directories(s_modspath);

    auto mount_gamepath_ok = PHYSFS_mount(s_gamepath.string().c_str(), MOUNT_DATA, false);
    vx::throw_if_not_fmt(mount_gamepath_ok, "failed to mount {}: {}", s_gamepath.string(), utils::physfs_error());

    auto mount_userpath_ok = PHYSFS_mount(s_userpath.string().c_str(), MOUNT_USER, false);
    vx::throw_if_not_fmt(mount_userpath_ok, "failed to mount {}: {}", s_userpath.string(), utils::physfs_error());

    auto mount_modspath_ok = PHYSFS_mount(s_modspath.string().c_str(), MOUNT_DATA, true);
    vx::throw_if_not_fmt(mount_modspath_ok, "failed to mount {}: {}", s_modspath.string(), utils::physfs_error());

    auto set_write_dir_ok = PHYSFS_setWriteDir(s_userpath.string().c_str());
    vx::throw_if_not_fmt(set_write_dir_ok, "failed to setwritedir {}: {}", s_userpath.string(), utils::physfs_error());

    Image::register_resource();

    threading::init();
}

void core::teardown(void)
{
    threading::shutdown();

    enet_deinitialize();

    auto physfs_deinit_ok = PHYSFS_deinit();
    vx::throw_if_not_fmt(physfs_deinit_ok, "failed to de-initialize physfs: {}", utils::physfs_error());
}
