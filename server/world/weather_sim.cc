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
#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"
#include "server/universe.hh"
#include "server/world/heightmap.hh"

constexpr static std::uint64_t CONTENT_TICK_INTERVAL = constant::FIXED_TICKRATE;

constexpr static float TYPE_CELL_SCALE = 1024.0f;
constexpr static float DETAIL_CELL_SCALE = 512.0f;

// The name each realm is saved under in
// world.conf; the order follows biome_realm
static const std::array REALM_NAMES = {
    std::string_view("surface"),     // BIOME_REALM_SURFACE
    std::string_view("underground"), // BIOME_REALM_UNDERGROUND
    std::string_view("depths"),      // BIOME_REALM_THE_DEPTHS
    std::string_view("sky"),         // BIOME_REALM_SKY
};

static_assert(NUM_BIOME_REALMS == REALM_NAMES.size());

static std::array<float, NUM_BIOME_REALMS> s_strike_budget {};
static std::array<Eigen::Vector2f, NUM_BIOME_REALMS> s_wind_direction {};
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

static Eigen::Vector2f realm_player_center(biome_realm realm)
{
    Eigen::Vector2f sum = Eigen::Vector2f::Zero();
    std::size_t count = 0;

    for(auto& session : sessions::all()) {
        if(session.state != session_state::CONNECTED || !globals::registry.valid(session.player)) {
            continue;
        }

        const auto& transform = globals::registry.get<Transform>(session.player);

        if(utils::realm(transform.chunk.y()) != realm) {
            continue;
        }

        auto world = utils::to_fvec(transform.chunk) + transform.local;
        sum += Eigen::Vector2f(world.x(), world.z());
        count += 1;
    }

    if(count == 0) {
        return Eigen::Vector2f::Zero();
    }

    return sum / static_cast<float>(count);
}

static void call_tick_callback(const WeatherCallback& callback, biome_realm realm)
{
    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);
    lua_pushinteger(L, static_cast<lua_Integer>(realm));
    lua_pushinteger(L, static_cast<lua_Integer>(globals::current_tick));

    if(lua_pcall(L, 2, 0, 0) != LUA_OK) {
        msg::log_error("weather on_tick: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void call_strike_callback(const WeatherCallback& callback, biome_realm realm, const BlockPos& bpos)
{
    if(callback.callback_ref == LUA_NOREF || callback.state == nullptr) {
        return;
    }

    auto L = callback.state.get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback.callback_ref);
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.x()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.z()));
    lua_pushinteger(L, static_cast<lua_Integer>(bpos.y()));
    lua_pushinteger(L, static_cast<lua_Integer>(realm));

    if(lua_pcall(L, 4, 0, 0) != LUA_OK) {
        msg::log_error("weather on_strike: {}", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static std::optional<biome_realm> parse_realm(std::string_view name)
{
    if(name == "surface") {
        return BIOME_REALM_SURFACE;
    }

    if(name == "sky") {
        return BIOME_REALM_SKY;
    }

    if(name == "underground") {
        return BIOME_REALM_UNDERGROUND;
    }

    if(name == "depths") {
        return BIOME_REALM_THE_DEPTHS;
    }

    return std::nullopt;
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

static void force_type(biome_realm realm, weather_type_id type)
{
    auto center = realm_player_center(realm);

    weather::force_at(realm, type, center.x(), center.y());
}

static cvar::status cmd_weather_force(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.size() < 2) {
        msg::log_error("usage: weather_force <surface|sky|underground|depths> <type>");
        return cvar::status::ARGUMENT;
    }

    auto realm = parse_realm(args[0]);

    if(!realm.has_value()) {
        msg::log_error("weather_force: unknown realm: {}", args[0]);
        return cvar::status::ARGUMENT;
    }

    auto type = weather_registry::resolve(args[1]);

    if(type == WEATHER_ID_NULL) {
        msg::log_error("weather_force: unknown weather: {}", args[1]);
        log_registered_types();
        return cvar::status::ARGUMENT;
    }

    force_type(realm.value(), type);

    auto center = realm_player_center(realm.value());
    auto coverage = weather::coverage(realm.value(), center.x(), center.y());

    msg::log_info("weather_force: {} is now {} (coverage {:.2f} at {:.0f}, {:.0f})", args[0], args[1], coverage, center.x(), center.y());
    return cvar::status::OK;
}

static void strike(biome_realm realm, const WeatherTypeDefinition& def)
{
    Session* target = nullptr;
    auto players = 0;

    for(auto& session : sessions::all()) {
        if(session.state != session_state::CONNECTED || !globals::registry.valid(session.player)) {
            continue;
        }

        const auto& transform = globals::registry.get<Transform>(session.player);

        if(utils::realm(transform.chunk.y()) != realm) {
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

    const auto& transform = globals::registry.get<Transform>(target->player);
    auto origin = utils::to_fvec(transform.chunk) + transform.local;

    auto angle = random_float() * std::numbers::pi_v<float> * 2.0f;
    auto distance = random_float() * def.strike_radius;
    auto x = static_cast<BlockPos::value_type>(std::floor(origin.x() + std::cos(angle) * distance));
    auto z = static_cast<BlockPos::value_type>(std::floor(origin.z() + std::sin(angle) * distance));

    // Only strike where the field has a storm
    // patch and the point still belongs to the same type
    if(weather::coverage(realm, static_cast<float>(x), static_cast<float>(z)) < 0.25f) {
        return;
    }

    auto column = heightmap::probe_slow(realm, BlockPosXZ(x, z));
    auto y = static_cast<BlockPos::value_type>(origin.y());

    // A strike lands on the surface; without a column to stand on it keeps the player's own height
    if(column.is_valid()) {
        y = column.surface_y + 1;
    }

    BlockPos bpos(x, y, z);

    msg::log_info("weather: strike at ({}, {}, {}) in realm {}", x, y, z, static_cast<unsigned>(realm));

    call_strike_callback(def.on_strike, realm, bpos);
}

static void update_realm(biome_realm realm)
{
    auto& state = weather::state(realm);

    auto center = realm_player_center(realm);
    auto def = weather_registry::find_definition(weather::type_at(realm, center.x(), center.y()));

    if(def == nullptr) {
        return;
    }

    // The front travels at the speed its own type asks for.
    // The direction is fixed per realm, so a type change
    // turns the speed up or down without the wind swinging around
    state.wind = s_wind_direction[static_cast<std::size_t>(realm)] * def->wind_speed;

    if(0 == (globals::current_tick % CONTENT_TICK_INTERVAL)) {
        call_tick_callback(def->on_tick, realm);
    }

    if(def->strikes_per_minute > 0.0f) {
        auto& budget = s_strike_budget[static_cast<std::size_t>(realm)];
        budget += def->strikes_per_minute * globals::fixed_frametime / 60.0f;

        while(budget >= 1.0f) {
            budget -= 1.0f;
            strike(realm, *def);
        }
    }
}

static std::string realm_key(biome_realm realm, std::string_view field)
{
    return std::format("weather.{}.{}", REALM_NAMES.at(static_cast<std::size_t>(realm)), field);
}

template<typename T>
static void save_value(biome_realm realm, std::string_view field, const T& value)
{
    auto key = realm_key(realm, field);

    universe::world_config().set_value<T>(key, value);
}

template<typename T>
static T load_value(biome_realm realm, std::string_view field, const T& fallback)
{
    auto key = realm_key(realm, field);

    return universe::world_config().value<T>(key).value_or(fallback);
}

// A world keeps its fronts in world.conf next to the seed, so a restart
// picks the sky up where it was left instead of rolling a new one. A world
// that has never been saved draws its seeds and its wind from the universe seed
static void load_state(void)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        auto realm = static_cast<biome_realm>(i);
        auto& state = weather::state(realm);

        state.types.drift = load_value<Eigen::Vector2f>(realm, "types_drift", Eigen::Vector2f::Zero());
        state.types.scale = load_value<float>(realm, "types_scale", TYPE_CELL_SCALE);
        state.types.seed = load_value<std::uint32_t>(realm, "types_seed", static_cast<std::uint32_t>(s_random()));

        state.detail.drift = load_value<Eigen::Vector2f>(realm, "detail_drift", Eigen::Vector2f::Zero());
        state.detail.scale = load_value<float>(realm, "detail_scale", DETAIL_CELL_SCALE);
        state.detail.seed = load_value<std::uint32_t>(realm, "detail_seed", static_cast<std::uint32_t>(s_random()));

        auto direction = load_value<Eigen::Vector2f>(realm, "wind_direction", Eigen::Vector2f::Zero());

        if(direction.squaredNorm() > 1.0e-6f) {
            s_wind_direction[i] = direction.normalized();
        }
        else {
            s_wind_direction[i] = random_direction();
        }

        state.wind = Eigen::Vector2f::Zero();
    }
}

static void save_state(void)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        auto realm = static_cast<biome_realm>(i);
        auto& state = weather::state(realm);

        save_value<Eigen::Vector2f>(realm, "types_drift", state.types.drift);
        save_value<float>(realm, "types_scale", state.types.scale);
        save_value<std::uint32_t>(realm, "types_seed", state.types.seed);

        save_value<Eigen::Vector2f>(realm, "detail_drift", state.detail.drift);
        save_value<float>(realm, "detail_scale", state.detail.scale);
        save_value<std::uint32_t>(realm, "detail_seed", state.detail.seed);

        save_value<Eigen::Vector2f>(realm, "wind_direction", s_wind_direction[i]);
    }
}

void weather_sim::init(void)
{
    auto& force_binding = cvar::bind_cmd("weather_force");
    force_binding.set_help("Force a weather type near the players");
    force_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    force_binding.connect<&cmd_weather_force>();

    s_random.seed(universe::seed() ^ UINT64_C(0x9E3779B97F4A7C15));
    s_strike_budget.fill(0.0f);
    weather::reset();

    load_state();
}

void weather_sim::shutdown(void)
{
    save_state();

    s_strike_budget.fill(0.0f);
    weather::reset();
}

void weather_sim::fixed_update(void)
{
    if(!gamerules::weather.value()) {
        return;
    }

    weather::advance(globals::fixed_frametime);

    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        update_realm(static_cast<biome_realm>(i));
    }
}
