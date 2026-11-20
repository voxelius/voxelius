#include "server/pch.hh"

#include "server/universe.hh"

#include "core/cmdline.hh"
#include "core/config/map.hh"
#include "core/config/ref.hh"
#include "core/core.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/utils/crc64.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/world/world.hh"
#include "shared/world/world_registry.hh"

#include "server/globals.hh"

static std::filesystem::path s_root_dir;
static std::filesystem::path s_conf_path;

static config::Map s_universe_config;
static config::Ref<std::uint64_t> s_seed { 0 };
static config::Ref<std::uint64_t> s_tick { 0 };
static config::Ref<std::uint64_t> s_daylight_tick { constant::DAY_LENGTH_TICKS / 4 };

static std::vector<std::shared_ptr<World>> s_worlds;
static std::shared_ptr<World> s_default_world;

static std::shared_ptr<World> find_world(const Identifier& id)
{
    for(const auto& world : s_worlds) {
        const auto& world_id = world->id();

        if(world_id == id) {
            return world;
        }
    }

    return nullptr;
}

std::span<const std::shared_ptr<World>> universe::worlds(void)
{
    return s_worlds;
}

std::shared_ptr<World> universe::find(const Identifier& id)
{
    return find_world(id);
}

const std::shared_ptr<World>& universe::default_world(void)
{
    return s_default_world;
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
    s_conf_path = s_root_dir / "universe.conf";

    std::filesystem::create_directories(s_root_dir);

    s_seed.bind(s_universe_config, "seed");
    s_tick.bind(s_universe_config, "tick");
    s_daylight_tick.bind(s_universe_config, "daylight_tick");

    std::ifstream config_stream(s_conf_path, std::ios::in);
    auto existing_save = config_stream.is_open();

    if(existing_save) {
        s_universe_config.load(config_stream);
        config_stream.close();
    }
    else {
        std::random_device noise;
        std::mt19937_64 seeder(noise());
        s_seed.set_value(seeder());
    }

    // The universe clock continues from the save; a universe
    // that has never been saved starts at noon
    globals::current_tick = s_tick.value();
    globals::daylight_tick = s_daylight_tick.value();

    auto definitions = world_registry::all();

    vx::throw_if(definitions.empty(), "no world is registered");

    s_worlds.clear();
    s_worlds.reserve(definitions.size());

    for(const auto& definition : definitions) {
        auto world = std::make_shared<World>(definition);
        auto full_id = definition->id.full_string();
        auto id_hash = utils::crc64(std::span<const char>(full_id.data(), full_id.size()));

        world->seed = s_seed.value() ^ id_hash;
        world->set_root(s_root_dir / definition->directory);

        std::filesystem::create_directories(world->chunk_dir());
        std::filesystem::create_directories(world->entity_dir());

        world->load_config();

        s_worlds.push_back(std::move(world));
    }

    auto configured_name = s_universe_config.value<std::string>("default_world");
    auto default_name = configured_name.value_or(std::string("builtin:overworld"));

    s_default_world = find_world(Identifier::from_string(default_name));

    if(s_default_world == nullptr) {
        s_default_world = s_worlds.front();
    }

    const auto& default_id = s_default_world->id();
    auto default_id_string = std::string(default_id.full_string());

    s_universe_config.set_value<std::string>("default_world", default_id_string);

    globals::world = s_default_world.get();
}

void universe::init_late(void)
{
    msg::log_info("set universe root={} worlds={}", s_root_dir.string(), s_worlds.size());

    for(const auto& world : s_worlds) {
        const auto& world_id = world->id();
        const auto& root = world->root();
        msg::log_info("world {} seed={} dir={}", world_id.full_string(), world->seed, root.string());
    }
}

void universe::shutdown(void)
{
    s_tick.set_value(globals::current_tick);
    s_daylight_tick.set_value(globals::daylight_tick);

    std::ofstream out(s_conf_path, std::ios::binary | std::ios::trunc);

    if(out.is_open()) {
        s_universe_config.save(out);
    }

    for(const auto& world : s_worlds) {
        world->save_config();
    }
}
