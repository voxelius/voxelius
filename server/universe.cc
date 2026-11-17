#include "server/pch.hh"

#include "server/universe.hh"

#include "core/cmdline.hh"
#include "core/config/map.hh"
#include "core/config/ref.hh"
#include "core/core.hh"

#include "server/globals.hh"

static std::filesystem::path s_root_dir;
static std::filesystem::path s_chunk_dir;
static std::filesystem::path s_entity_dir;
static std::filesystem::path s_conf_path;

static config::Map s_world_config;
static config::Ref<std::uint64_t> s_seed { 0 };

config::Map& universe::world_config(void)
{
    return s_world_config;
}

std::uint64_t universe::seed(void)
{
    return s_seed.value();
}

void universe::init(void)
{
    auto default_path = std::filesystem::path(core::userpath() / "save").string();
    auto argument = cmdline::value_or("universe", default_path);
    auto argument_path = std::filesystem::absolute(argument);

    s_root_dir = argument_path;
    s_chunk_dir = s_root_dir / "chunk";
    s_entity_dir = s_root_dir / "entity";
    s_conf_path = s_root_dir / "world.conf";

    std::filesystem::create_directories(s_chunk_dir);
    std::filesystem::create_directories(s_entity_dir);

    s_seed.bind(s_world_config, "seed");

    std::ifstream config_stream(s_conf_path, std::ios::in);
    auto existing_save = config_stream.is_open();

    if(existing_save) {
        s_world_config.load(config_stream);
        config_stream.close();
    }
    else {
        std::random_device noise;
        std::mt19937_64 seeder(noise());
        s_seed.set_value(seeder());
    }
}

void universe::init_late(void)
{
    msg::log_info("set world root={}", s_root_dir.string());
    msg::log_info("set world seed={}", s_seed.value());
}

void universe::shutdown(void)
{
    std::ofstream out(s_conf_path, std::ios::binary | std::ios::trunc);

    if(out.is_open()) {
        s_world_config.save(out);
    }
}

const std::filesystem::path& universe::chunk_dir(void)
{
    return s_chunk_dir;
}

const std::filesystem::path& universe::entity_dir(void)
{
    return s_entity_dir;
}
