#include "server/pch.hh"

#include "server/universe.hh"

#include "core/config/map.hh"
#include "core/config/ref.hh"
#include "core/core.hh"
#include "core/exception.hh"
#include "core/utils/crc64.hh"
#include "core/utils/physfs.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/world/columns.hh"
#include "server/world/world.hh"
#include "server/world/world_loader.hh"

constexpr static std::string_view UNIVERSE_CONFIG = "save/universe.conf";

static std::filesystem::path s_root_dir;

static config::Map s_universe_config;
static config::Ref<std::uint64_t> s_seed { 0 };
static config::Ref<std::uint64_t> s_tick { 0 };

static std::vector<std::shared_ptr<ServerWorld>> s_worlds;
static std::shared_ptr<ServerWorld> s_default_world;

static std::shared_ptr<ServerWorld> find_world(std::string_view name)
{
    for(const auto& world : s_worlds) {
        auto& world_name = world->name();

        if(world_name == name) {
            return world;
        }
    }

    return nullptr;
}

std::span<const std::shared_ptr<ServerWorld>> universe::worlds(void)
{
    return s_worlds;
}

std::shared_ptr<ServerWorld> universe::find(std::string_view name)
{
    return find_world(name);
}

const std::shared_ptr<ServerWorld>& universe::default_world(void)
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

    s_root_dir = std::filesystem::absolute(default_dir);

    std::filesystem::create_directories(s_root_dir);

    s_seed.bind(s_universe_config, "seed");
    s_tick.bind(s_universe_config, "tick");

    std::istringstream config_stream;

    if(utils::read_file(UNIVERSE_CONFIG, config_stream)) {
        s_universe_config.load(config_stream);
    }
    else {
        std::random_device noise;
        std::mt19937_64 seeder(noise());
        s_seed.set_value(seeder());
    }

    // The universe clock continues from the save; a universe
    // that has never been saved starts at noon
    globals::current_tick = s_tick.value();

    columns::init();

    auto definitions = world_loader::load();

    s_worlds.clear();
    s_worlds.reserve(definitions.size());
    s_default_world.reset();

    for(auto& definition : definitions) {
        // The seed keeps the builtin-qualified name so that worlds
        // saved before the plain-name keys keep their terrain unchanged
        auto seed_text = std::format("{}:{}", constant::BUILTIN_NAME_SPACE, definition->name);
        auto bytes = std::span<const char>(seed_text);
        auto id_hash = utils::crc64(bytes);

        auto world = std::make_shared<ServerWorld>(definition);

        world->seed = s_seed.value() ^ id_hash;
        world->set_root(s_root_dir / definition->name);

        std::filesystem::create_directories(world->chunk_dir());
        std::filesystem::create_directories(world->entity_dir());

        world->columns().init(world.get());
        world->load_config();
        world->init_generation();

        if(definition->is_default) {
            s_default_world = world;
        }

        s_worlds.push_back(std::move(world));
    }

    vx::throw_if(s_default_world == nullptr, "worlds.jsonc: no default world");
}

void universe::init_late(void)
{
    auto root_text = s_root_dir.string();
    auto& default_text = s_default_world->name();

    msg::log_info("set universe root={} worlds={} default={}", root_text, s_worlds.size(), default_text);

    for(const auto& world : s_worlds) {
        auto& world_name = world->name();
        auto& root = world->root();
        auto dir_text = root.string();

        msg::log_info("world {} seed={} dir={}", world_name, world->seed, dir_text);
    }
}

void universe::shutdown(void)
{
    s_tick.set_value(globals::current_tick);

    columns::shutdown();

    for(const auto& world : s_worlds) {
        world->columns().save();
    }

    std::ostringstream config_stream;
    s_universe_config.save(config_stream);

    auto written = utils::write_file(UNIVERSE_CONFIG, config_stream.str());

    if(!written) {
        msg::log_warning("{}: unable to save the universe configuration", UNIVERSE_CONFIG);
    }

    for(const auto& world : s_worlds) {
        world->save_config();
    }
}
