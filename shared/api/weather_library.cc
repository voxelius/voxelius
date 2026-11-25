#include "shared/pch.hh"

#include "shared/api/weather_library.hh"

#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/precipitation.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_weather(candidate) == WEATHER_ID_NULL && weather_registry::find(candidate) == WEATHER_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto suffix = std::format("{}~{}", id.value(), n);
        auto candidate = Identifier::from_parts(id.name_space(), suffix);

        if(is_free(candidate)) {
            msg::log_warning("weather.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("weather.add: {}: out of resolvable names", id.full_string());
    return id;
}

static WeatherCallback parse_callback(lua_State* L, int table_idx, const char* key, ModContext* ctx)
{
    lua_getfield(L, table_idx, key);

    if(lua_isfunction(L, -1)) {
        WeatherCallback callback {};
        callback.state = ctx->lua_state();
        callback.callback_ref = luaL_ref(L, LUA_REGISTRYINDEX); // Pops the function
        return callback;
    }

    lua_pop(L, 1);
    return {};
}

static bool parse_color(lua_State* L, int table_idx, const char* key, Eigen::Vector3f& out)
{
    Eigen::Vector<lua_Number, 3> default_value(out.x(), out.y(), out.z());

    auto color = utils::opt_fvec<3>(L, table_idx, key, default_value);

    if(!color.has_value()) {
        return false;
    }

    out = color->cast<float>();
    return true;
}

// Drops whatever the failing helper pushed above `base`
// while keeping the error text it left on top, so a rejected
// definition does not leave the Lua stack deeper than it found it
static bool fail(lua_State* L, int base)
{
    std::string message = "invalid value";

    if(LUA_TSTRING == lua_type(L, -1)) {
        message = lua_tostring(L, -1);
    }

    lua_settop(L, base);
    lua_pushlstring(L, message.data(), message.size());

    return false;
}

static bool fail(lua_State* L, int base, std::string_view message)
{
    lua_settop(L, base);
    lua_pushlstring(L, message.data(), message.size());

    return false;
}

static bool parse_cloud_body(lua_State* L, int cloud_idx, CloudBody& cloud)
{
    auto height = utils::opt_number(L, cloud_idx, "height", cloud.height);

    if(!height.has_value()) {
        return false;
    }

    cloud.height = static_cast<float>(height.value());

    auto thickness = utils::opt_number(L, cloud_idx, "thickness", cloud.thickness);

    if(!thickness.has_value()) {
        return false;
    }

    cloud.thickness = std::clamp(static_cast<float>(thickness.value()), 1.0f, 128.0f);

    auto opacity = utils::opt_number(L, cloud_idx, "opacity", cloud.opacity);

    if(!opacity.has_value()) {
        return false;
    }

    cloud.opacity = std::clamp(static_cast<float>(opacity.value()), 0.0f, 1.0f);

    auto scale = utils::opt_number(L, cloud_idx, "scale", cloud.scale);

    if(!scale.has_value()) {
        return false;
    }

    cloud.scale = std::max(1.0f, static_cast<float>(scale.value()));

    auto coverage = utils::opt_number(L, cloud_idx, "coverage", cloud.coverage);

    if(!coverage.has_value()) {
        return false;
    }

    cloud.coverage = std::clamp(static_cast<float>(coverage.value()), 0.0f, 1.0f);

    auto seed = utils::opt_integer(L, cloud_idx, "seed", static_cast<lua_Integer>(cloud.seed));

    if(!seed.has_value()) {
        return false;
    }

    cloud.seed = static_cast<std::uint32_t>(seed.value());

    return parse_color(L, cloud_idx, "shadow", cloud.shadow_color) && parse_color(L, cloud_idx, "lit", cloud.lit_color);
}

static bool parse_cloud(lua_State* L, int def_idx, WeatherTypeDefinition& def)
{
    auto base = lua_gettop(L);

    lua_getfield(L, def_idx, "cloud");

    if(lua_isnoneornil(L, -1)) {
        lua_settop(L, base);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, "cloud must be a table");
    }

    CloudBody cloud {};

    auto cloud_idx = lua_gettop(L);

    if(!parse_cloud_body(L, cloud_idx, cloud)) {
        return fail(L, base);
    }

    def.cloud = cloud;

    lua_settop(L, base);
    return true;
}

static bool parse_precipitation(lua_State* L, int def_idx, WeatherTypeDefinition& def, ModContext* ctx)
{
    auto base = lua_gettop(L);

    lua_getfield(L, def_idx, "precipitation");

    if(lua_isnoneornil(L, -1)) {
        lua_settop(L, base);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, "precipitation must be a table");
    }

    auto index = lua_gettop(L);

    lua_getfield(L, index, "types");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            return fail(L, base, "types must be a table");
        }

        auto types_index = lua_gettop(L);
        auto count = lua_rawlen(L, types_index);

        for(std::size_t i = 0; i < count; ++i) {
            lua_rawgeti(L, types_index, static_cast<lua_Integer>(i + 1));

            auto value = utils::require_string(L, -1);

            if(!value.has_value()) {
                return fail(L, base, "types must hold strings");
            }

            auto name = Identifier::from_string(value.value(), ctx->name_space());

            if(!name.is_valid()) {
                return fail(L, base, "types must hold valid identifiers");
            }

            def.precipitation.type_names.push_back(name);
            lua_pop(L, 1);
        }

        lua_settop(L, index);
    }
    else {
        lua_settop(L, index);
    }

    auto intensity = utils::opt_number(L, index, "intensity", def.precipitation.intensity);

    if(!intensity.has_value()) {
        return fail(L, base);
    }

    def.precipitation.intensity = static_cast<float>(intensity.value());

    lua_settop(L, base);
    return true;
}

static bool parse_definition(lua_State* L, int def_idx, WeatherTypeDefinition& def, ModContext* ctx)
{
    auto base = lua_gettop(L);

    const std::array numbers = {
        std::make_pair("intensity", &def.intensity),
        std::make_pair("sky_visibility", &def.sky_visibility),
        std::make_pair("light", &def.light_scale),
        std::make_pair("strikes_per_minute", &def.strikes_per_minute),
        std::make_pair("strike_radius", &def.strike_radius),
    };

    for(const auto& [field_name, field_ptr] : numbers) {
        auto value = utils::opt_number(L, def_idx, field_name, *field_ptr);

        if(!value.has_value()) {
            return false;
        }

        field_ptr[0] = static_cast<float>(value.value());
    }

    def.sky_visibility = std::clamp(def.sky_visibility, 0.0f, 1.0f);

    // The weight stretches the type's cell around its target,
    // which is the only thing that makes one weather more common than another
    auto weight = utils::opt_number(L, def_idx, "weight", static_cast<lua_Number>(def.weight));

    if(!weight.has_value()) {
        return fail(L, base, "weight must be a number");
    }

    if(weight.value() <= 0.0) {
        return fail(L, base, "weight must be positive");
    }

    def.weight = static_cast<float>(weight.value());

    // The target places the type in the two-axis climate square that
    // the two noise grids span; the nearest target wins at a point
    auto target = utils::opt_fvec<2>(L, def_idx, "target", Eigen::Vector<lua_Number, 2>(def.target.x(), def.target.y()));

    if(!target.has_value()) {
        return fail(L, base, "target must hold two numbers");
    }

    if(target->minCoeff() < 0.0 || target->maxCoeff() > 1.0) {
        return fail(L, base, "target must be in 0 to 1 on both axes");
    }

    def.target = target->cast<float>();

    def.on_tick = parse_callback(L, def_idx, "on_tick", ctx);
    def.on_strike = parse_callback(L, def_idx, "on_strike", ctx);

    return parse_cloud(L, def_idx, def) && parse_precipitation(L, def_idx, def, ctx) && parse_color(L, def_idx, "sky_tint", def.sky_tint);
}

static std::optional<weather_type_id> add_definition(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "weather.add: malformed name: %s", raw_name);
        return std::nullopt;
    }

    id = make_unique_id(id, ctx);

    WeatherTypeDefinition def {};
    def.name = id;

    if(!parse_definition(L, def_idx, def, ctx)) {
        return std::nullopt;
    }

    return ctx->register_weather(id, std::move(def));
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto raw_name = luaL_checkstring(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);

    auto id = add_definition(L, ctx, raw_name, 2);

    if(!id.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(id.value()));
    return 1;
}

static bool state(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return false;
    }

    auto& weather_state = world->weather();

    lua_newtable(L);

    lua_pushinteger(L, static_cast<lua_Integer>(weather_state.seed()));
    lua_setfield(L, -2, "seed");

    lua_pushinteger(L, static_cast<lua_Integer>(world->weather_tick));
    lua_setfield(L, -2, "tick");

    auto wind = weather_state.wind_direction() * weather_state.wind_speed();

    lua_pushnumber(L, static_cast<lua_Number>(wind.x()));
    lua_setfield(L, -2, "wind_x");

    lua_pushnumber(L, static_cast<lua_Number>(wind.y()));
    lua_setfield(L, -2, "wind_z");

    lua_pushnumber(L, static_cast<lua_Number>(weather_state.wind_speed()));
    lua_setfield(L, -2, "wind_speed");

    return true;
}

static int api_state(lua_State* L)
{
    if(!state(L)) {
        return lua_error(L);
    }

    return 1;
}

static bool sample(lua_State* L)
{
    auto x = luaL_checknumber(L, 1);
    auto z = luaL_checknumber(L, 2);

    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return false;
    }

    auto& definition = world->definition();
    auto& weather_state = world->weather();
    auto& allowed = definition.weather;

    auto coverage = weather_state.coverage(allowed, static_cast<float>(x), static_cast<float>(z), world->weather_tick);
    auto type = weather_state.type_at(allowed, static_cast<float>(x), static_cast<float>(z), world->weather_tick);

    lua_newtable(L);

    lua_pushnumber(L, static_cast<lua_Number>(coverage));
    lua_setfield(L, -2, "coverage");

    lua_pushinteger(L, static_cast<lua_Integer>(type));
    lua_setfield(L, -2, "type");

    auto def = weather_registry::find_definition(type);

    if(def && !def->precipitation.types.empty()) {
        lua_pushnumber(L, static_cast<lua_Number>(def->precipitation.intensity * coverage));
    }
    else {
        lua_pushnumber(L, 0.0);
    }

    lua_setfield(L, -2, "precipitation");

    return true;
}

static int api_sample(lua_State* L)
{
    if(!sample(L)) {
        return lua_error(L);
    }

    return 1;
}

static bool precipitation_at(lua_State* L)
{
    auto x = luaL_checknumber(L, 1);
    auto y = luaL_checknumber(L, 2);
    auto z = luaL_checknumber(L, 3);

    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return false;
    }

    auto& definition = world->definition();
    auto& allowed = definition.weather;

    auto precip_ctx = precipitation::context(world, allowed, static_cast<float>(x), static_cast<float>(z));
    auto selection = precipitation::select(world, precip_ctx, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

    lua_newtable(L);

    lua_pushinteger(L, static_cast<lua_Integer>(selection.type));
    lua_setfield(L, -2, "type");

    lua_pushnumber(L, static_cast<lua_Number>(selection.effective_intensity));
    lua_setfield(L, -2, "amount");

    return true;
}

static int api_precipitation_at(lua_State* L)
{
    if(!precipitation_at(L)) {
        return lua_error(L);
    }

    return 1;
}

void api_shared::open_weather(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L);

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_pushcfunction(L, &api_state);
    lua_setfield(L, -2, "state");

    lua_pushcfunction(L, &api_sample);
    lua_setfield(L, -2, "sample");

    lua_pushcfunction(L, &api_precipitation_at);
    lua_setfield(L, -2, "precipitation_at");

    lua_setglobal(L, "weather");
}
