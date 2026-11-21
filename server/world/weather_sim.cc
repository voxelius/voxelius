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

constexpr static float TYPE_CELL_SCALE = 1024.0f;
constexpr static float DETAIL_CELL_SCALE = 512.0f;

static std::mt19937_64 s_random;

static float random_float(void)
{
    auto value = s_random();
    return static_cast<float>(value >> 11) / static_cast<float>(1ULL << 53);
}

static Eigen::Vector2f random_direction(void)
{
    auto angle = random_float() * std::numbers::pi_v<float> * 2.0f;

    return Eigen::Vector2f(std::cos(angle), std::sin(angle));
}

static Eigen::Vector2f player_center(World* world)
{
    auto& registry = world->registry();

    Eigen::Vector2f sum = Eigen::Vector2f::Zero();
    std::size_t count = 0;

    for(auto& session : sessions::all()) {
        if(session.world.get() != world) {
            continue;
        }

        if(session.state != session_state::CONNECTED || !registry.valid(session.player)) {
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

static cvar::status cmd_weather_force(const cvar::Context&, std::span<const std::string_view> args)
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

    auto& default_world = universe::default_world();

    auto center = player_center(default_world.get());
    auto& definition = default_world->definition();
    auto& state = default_world->weather();
    auto& allowed = definition.weather;

    if(!state.force_at(allowed, type, center.x(), center.y())) {
        msg::log_error("weather_force: {} is not part of this world", args[0]);

        return cvar::status::ARGUMENT;
    }

    auto coverage = state.coverage(allowed, center.x(), center.y());

    msg::log_info("weather_force: {} is now active (coverage {:.2f} at {:.0f}, {:.0f})", args[0], coverage, center.x(), center.y());
    return cvar::status::OK;
}

static void strike(ServerWorld* world, const WeatherTypeDefinition& def)
{
    auto& registry = world->registry();

    Session* target = nullptr;
    auto players = 0;

    for(auto& session : sessions::all()) {
        if(session.world.get() != world) {
            continue;
        }

        if(session.state != session_state::CONNECTED || !registry.valid(session.player)) {
            continue;
        }

        players += 1;

        if(0 == (s_random() % static_cast<std::uint64_t>(players))) {
            target = &session;
        }
    }

    if(target == nullptr) {
        return;
    }

    const auto& transform = registry.get<Transform>(target->player);
    auto origin = utils::to_fvec(transform.chunk) + transform.local;

    auto angle = random_float() * std::numbers::pi_v<float> * 2.0f;
    auto distance = random_float() * def.strike_radius;
    auto x = static_cast<BlockPos::value_type>(std::floor(origin.x() + std::cos(angle) * distance));
    auto z = static_cast<BlockPos::value_type>(std::floor(origin.z() + std::sin(angle) * distance));

    // Only strike where the field has a storm
    // patch and the point still belongs to the same type
    auto& definition = world->definition();
    auto& state = world->weather();
    auto& allowed = definition.weather;

    if(state.coverage(allowed, static_cast<float>(x), static_cast<float>(z)) < 0.25f) {
        return;
    }

    auto column = utils::heightmap_probe_slow(world, BlockPosXZ(x, z));
    auto y = static_cast<BlockPos::value_type>(origin.y());

    // A strike lands on the surface; without a column to stand on it keeps the player's own height
    if(column.is_valid()) {
        y = column.surface_y + 1;
    }

    BlockPos bpos(x, y, z);

    msg::log_info("weather: strike at ({}, {}, {})", x, y, z);

    call_strike_callback(def.on_strike, bpos);
}

static void update_world(ServerWorld* world)
{
    auto& definition = world->definition();
    auto& state = world->weather();
    auto& allowed = definition.weather;

    auto center = player_center(world);
    auto def = weather_registry::find_definition(state.type_at(allowed, center.x(), center.y()));

    if(def == nullptr) {
        return;
    }

    // The front travels at the speed its own type asks for.
    // The direction is fixed per world, so a type change
    // turns the speed up or down without the wind swinging around
    state.wind = state.wind_direction * def->wind_speed;

    if(0 == (globals::current_tick % CONTENT_TICK_INTERVAL)) {
        call_tick_callback(def->on_tick);
    }

    if(def->strikes_per_minute > 0.0f) {
        state.strike_budget += def->strikes_per_minute * globals::fixed_frametime / 60.0f;

        while(state.strike_budget >= 1.0f) {
            state.strike_budget -= 1.0f;
            strike(world, *def);
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

    return config.value<T>(key).value_or(fallback);
}

// A world keeps its fronts in world.conf next to the seed, so a restart
// picks the sky up where it was left instead of rolling a new one. A world
// that has never been saved draws its seeds and its wind from the universe seed
static void load_state(World* world)
{
    auto& state = world->weather();

    state.types.drift = load_value<Eigen::Vector2f>(world, "types_drift", Eigen::Vector2f::Zero());
    state.types.scale = load_value<float>(world, "types_scale", TYPE_CELL_SCALE);
    state.types.seed = load_value<std::uint32_t>(world, "types_seed", static_cast<std::uint32_t>(s_random()));

    state.detail.drift = load_value<Eigen::Vector2f>(world, "detail_drift", Eigen::Vector2f::Zero());
    state.detail.scale = load_value<float>(world, "detail_scale", DETAIL_CELL_SCALE);
    state.detail.seed = load_value<std::uint32_t>(world, "detail_seed", static_cast<std::uint32_t>(s_random()));

    auto direction = load_value<Eigen::Vector2f>(world, "wind_direction", Eigen::Vector2f::Zero());

    if(direction.squaredNorm() > 1.0e-6f) {
        state.wind_direction = direction.normalized();
    }
    else {
        state.wind_direction = random_direction();
    }

    state.wind = Eigen::Vector2f::Zero();
    state.strike_budget = 0.0f;
}

static void save_state(World* world)
{
    auto& state = world->weather();

    save_value<Eigen::Vector2f>(world, "types_drift", state.types.drift);
    save_value<float>(world, "types_scale", state.types.scale);
    save_value<std::uint32_t>(world, "types_seed", state.types.seed);

    save_value<Eigen::Vector2f>(world, "detail_drift", state.detail.drift);
    save_value<float>(world, "detail_scale", state.detail.scale);
    save_value<std::uint32_t>(world, "detail_seed", state.detail.seed);

    save_value<Eigen::Vector2f>(world, "wind_direction", state.wind_direction);
}

void weather_sim::init(void)
{
    auto& force_binding = cvar::bind_cmd("weather_force");
    force_binding.set_help("Force a weather type near the players");
    force_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    force_binding.connect<&cmd_weather_force>();

    for(const auto& world : universe::worlds()) {
        s_random.seed(world->seed ^ UINT64_C(0x9E3779B97F4A7C15));

        auto& state = world->weather();
        state.reset();

        load_state(world.get());
    }
}

void weather_sim::shutdown(void)
{
    for(const auto& world : universe::worlds()) {
        save_state(world.get());
    }
}

void weather_sim::fixed_update(void)
{
    if(!gamerules::weather.value()) {
        return;
    }

    for(const auto& world : universe::worlds()) {
        WorldScope scope(world.get());

        auto& state = world->weather();
        state.advance(globals::fixed_frametime);

        update_world(world.get());
    }
}
