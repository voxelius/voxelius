#include "server/pch.hh"

#include "server/world/weather_sim.hh"

#include "core/config/map.hh"
#include "core/cvar/context.hh"
#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "shared/component/transform.hh"
#include "shared/constant.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"
#include "server/universe.hh"
#include "server/utils/heightmap.hh"
#include "server/world/world.hh"

constexpr static std::uint64_t CONTENT_TICK_INTERVAL = constant::FIXED_TICKRATE;

constexpr static float STRIKE_COVERAGE = 0.25f;

static float random_float(std::mt19937_64& random)
{
    auto value = random();
    return static_cast<float>(value >> 11) / static_cast<float>(1ULL << 53);
}

static Eigen::Vector2f player_center(World* world)
{
    auto& registry = world->registry();

    Eigen::Vector2f sum = Eigen::Vector2f::Zero();
    std::size_t count = 0;

    for(auto& session : sessions::all()) {
        if(session.state != session_state::CONNECTED || session.peer == nullptr) {
            continue;
        }

        if(session.world.get() != world) {
            continue;
        }

        if(!registry.valid(session.player)) {
            continue;
        }

        const auto& transform = registry.get<Transform>(session.player);
        auto position = utils::to_fvec(transform.chunk) + transform.local;
        sum += Eigen::Vector2f(position.x(), position.z());
        count += 1;
    }

    if(count == 0) {
        return Eigen::Vector2f::Zero();
    }

    return sum / static_cast<float>(count);
}

static void call_tick_callback(const WeatherCallback& callback)
{
    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);
    lua_pushinteger(L, static_cast<lua_Integer>(globals::current_tick));

    if(lua_pcall(L, 1, 0, 0) != LUA_OK) {
        msg::log_error("weather on_tick: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void call_strike_callback(const WeatherCallback& callback, const BlockPos& bpos)
{
    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.z()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.y()));

    if(lua_pcall(L, 3, 0, 0) != LUA_OK) {
        msg::log_error("weather on_strike: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void log_registered_types(void)
{
    std::string names;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        if(!names.empty()) {
            names += ", ";
        }

        names += def.name.full_string();
    }

    msg::log_error("weather_force: registered types: {}", names);
}

static cvar::status cmd_weather_force(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_error("usage: weather_force <type>");
        return cvar::status::ARGUMENT;
    }

    auto type = weather_registry::resolve(args[0]);

    if(type == WEATHER_ID_NULL) {
        msg::log_error("weather_force: unknown weather: {}", args[0]);
        log_registered_types();
        return cvar::status::ARGUMENT;
    }

    auto world = universe::default_world();

    if(ctx.client_id) {
        auto session = sessions::find(*ctx.client_id);

        if(session && session->world) {
            world = std::static_pointer_cast<ServerWorld>(session->world);
        }
    }

    auto center = player_center(world.get());
    auto& definition = world->definition();
    auto& state = world->weather();
    auto& allowed = definition.weather;

    auto tick = state.solve_force(allowed, type, center.x(), center.y(), world->weather_tick);

    if(!tick.has_value()) {
        msg::log_error("weather_force: {} cannot reach the players in this world", args[0]);

        return cvar::status::ARGUMENT;
    }

    world->weather_tick = tick.value();

    auto coverage = state.coverage(allowed, center.x(), center.y(), world->weather_tick);

    msg::log_info("weather_force: {} is now active (coverage {:.2f} at {:.0f}, {:.0f}, weather tick {})", args[0], coverage, center.x(),
        center.y(), world->weather_tick);
    return cvar::status::OK;
}

static void strike(ServerWorld* world, const WeatherTypeDefinition* def)
{
    auto& registry = world->registry();

    Session* target = nullptr;
    auto players = 0;

    for(auto& session : sessions::all()) {
        if(session.state != session_state::CONNECTED || session.peer == nullptr) {
            continue;
        }

        if(session.world.get() != world) {
            continue;
        }

        if(!registry.valid(session.player)) {
            continue;
        }

        players += 1;

        if(0 == (world->weather_random() % static_cast<std::uint64_t>(players))) {
            target = &session;
        }
    }

    if(target == nullptr) {
        return;
    }

    const auto& transform = registry.get<Transform>(target->player);
    auto origin = utils::to_fvec(transform.chunk) + transform.local;

    auto angle = random_float(world->weather_random) * std::numbers::pi_v<float> * 2.0f;
    auto distance = random_float(world->weather_random) * def->strike_radius;
    auto x = static_cast<BlockPos::value_type>(std::floor(origin.x() + std::cos(angle) * distance));
    auto z = static_cast<BlockPos::value_type>(std::floor(origin.z() + std::sin(angle) * distance));

    // Only strike where the field has a storm
    // patch and the point still belongs to the same type
    auto& definition = world->definition();
    auto& state = world->weather();
    auto& allowed = definition.weather;

    if(state.coverage(allowed, static_cast<float>(x), static_cast<float>(z), world->weather_tick) < STRIKE_COVERAGE) {
        return;
    }

    auto column = utils::heightmap_probe_slow(world, BlockPosXZ(x, z));
    auto y = static_cast<BlockPos::value_type>(origin.y());

    if(column.is_valid()) {
        y = column.surface_y + 1;
    }

    BlockPos bpos(x, y, z);

    msg::log_info("weather: strike at ({}, {}, {})", x, y, z);

    call_strike_callback(def->on_strike, bpos);
}

static void update_world(ServerWorld* world)
{
    auto& definition = world->definition();
    auto& state = world->weather();
    auto& allowed = definition.weather;

    auto center = player_center(world);
    auto def = weather_registry::find_definition(state.type_at(allowed, center.x(), center.y(), world->weather_tick));

    if(def == nullptr) {
        return;
    }

    if(0 == (globals::current_tick % CONTENT_TICK_INTERVAL)) {
        call_tick_callback(def->on_tick);
    }

    if(def->strikes_per_minute > 0.0f) {
        world->weather_strike_budget += def->strikes_per_minute * globals::fixed_frametime / 60.0f;

        while(world->weather_strike_budget >= 1.0f) {
            world->weather_strike_budget -= 1.0f;
            strike(world, def);
        }
    }
}

static std::string weather_key(std::string_view field)
{
    return std::format("weather.{}", field);
}

template<typename T>
static void save_value(World* world, std::string_view field, const T& value)
{
    auto key = weather_key(field);
    auto& config = world->config();

    config.set_value<T>(key, value);
}

template<typename T>
static T load_value(World* world, std::string_view field, const T& fallback)
{
    auto key = weather_key(field);
    auto& config = world->config();

    auto stored = config.value<T>(key);

    return stored.value_or(fallback);
}

// A world keeps its weather seed in world.conf next to its clocks, so a
// restart continues the same fronts. A world that has never been saved
// rolls one now, independently of the seed the terrain came from
static std::uint64_t load_or_roll_seed(World* world)
{
    auto key = weather_key("seed");
    auto& config = world->config();
    auto stored = config.value<std::uint64_t>(key);

    if(stored.has_value()) {
        return stored.value();
    }

    std::random_device noise;
    std::mt19937_64 seeder(noise());

    return seeder();
}

void weather_sim::init(void)
{
    auto& force_binding = cvar::bind_cmd("weather_force");
    force_binding.set_help("Force a weather type near the players");
    force_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    force_binding.connect<&cmd_weather_force>();

    for(const auto& world : universe::worlds()) {
        auto seed = load_or_roll_seed(world.get());

        world->set_weather_seed(seed);
        world->weather_tick = load_value<std::uint64_t>(world.get(), "tick", 0);
        world->weather_random.seed(seed);
        world->weather_strike_budget = 0.0f;
    }
}

void weather_sim::shutdown(void)
{
    for(const auto& world : universe::worlds()) {
        save_value<std::uint64_t>(world.get(), "seed", world->weather_seed);
        save_value<std::uint64_t>(world.get(), "tick", world->weather_tick);
    }
}

void weather_sim::fixed_update(void)
{
    if(!gamerules::weather.value()) {
        return;
    }

    for(const auto& world : universe::worlds()) {
        WorldScope scope(world.get());

        world->weather_tick += 1;

        update_world(world.get());
    }
}
