#include "server/pch.hh"

#include "server/universe.hh"

#include "core/cmdline.hh"
#include "core/config/map.hh"
#include "core/config/ref.hh"
#include "core/core.hh"
#include "core/exception.hh"
#include "core/identifier.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/world/world.hh"
#include "shared/world/world_registry.hh"

#include "server/globals.hh"

static std::filesystem::path s_root_dir;
static std::filesystem::path s_chunk_dir;
static std::filesystem::path s_entity_dir;
static std::filesystem::path s_conf_path;

static config::Map s_world_config;
static config::Ref<std::uint64_t> s_seed { 0 };
static config::Ref<std::uint64_t> s_tick { 0 };
static config::Ref<std::uint64_t> s_daylight_tick { constant::DAY_LENGTH_TICKS / 4 };

config::Map& universe::world_config(void)
{
    return s_world_config;
}

World* universe::world(void)
{
    assert(globals::world);
    return globals::world.get();
}

std::uint64_t universe::seed(void)
{
    return s_seed.value();
}

void universe::init(void)
{
    auto default_dir = core::userpath() / "save";
    auto default_path = default_dir.string();
    auto argument = cmdline::value_or("universe", default_path);
    auto argument_path = std::filesystem::absolute(argument);

    s_root_dir = argument_path;
    s_chunk_dir = s_root_dir / "chunk";
    s_entity_dir = s_root_dir / "entity";
    s_conf_path = s_root_dir / "world.conf";

    std::filesystem::create_directories(s_chunk_dir);
    std::filesystem::create_directories(s_entity_dir);

    s_seed.bind(s_world_config, "seed");
    s_tick.bind(s_world_config, "tick");
    s_daylight_tick.bind(s_world_config, "daylight_tick");

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

    // The world clock continues from the save; a world
    // that has never been saved starts at noon
    globals::current_tick = s_tick.value();
    globals::daylight_tick = s_daylight_tick.value();

    auto definitions = world_registry::all();
    vx::throw_if(definitions.empty(), "no world is registered");

    auto definition = world_registry::find(Identifier::from_string("builtin:overworld"));

    if(definition == nullptr) {
        definition = definitions.front();
    }

    globals::world = std::make_shared<World>(definition);
    globals::world->seed = s_seed.value();
}

void universe::init_late(void)
{
    const auto& world_id = globals::world->id();
    msg::log_info("set world root={}", s_root_dir.string());
    msg::log_info("set world id={} seed={}", world_id.full_string(), s_seed.value());
}

void universe::shutdown(void)
{
    s_tick.set_value(globals::current_tick);
    s_daylight_tick.set_value(globals::daylight_tick);

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
