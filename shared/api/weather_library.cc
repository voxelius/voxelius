#include "shared/pch.hh"

#include "shared/api/weather_library.hh"

#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/globals.hh"
#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_weather(candidate) == WEATHER_ID_NULL && weather_registry::find(candidate) == WEATHER_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto candidate = Identifier::from_parts(id.name_space(), std::format("{}~{}", id.value(), n));

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
        callback.callback_ref = luaL_ref(L, LUA_REGISTRYINDEX); // pops the function
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

    if(!parse_cloud_body(L, lua_gettop(L), cloud)) {
        return fail(L, base);
    }

    def.cloud = cloud;

    lua_settop(L, base);
    return true;
}

static bool parse_precipitation(lua_State* L, int def_idx, WeatherTypeDefinition& def)
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

    auto kind = utils::opt_string(L, index, "kind", "none");

    if(!kind.has_value()) {
        return fail(L, base);
    }

    if(0 == kind->compare("rain")) {
        def.precipitation.kind = precipitation_kind::RAIN;
    }
    else if(0 == kind->compare("snow")) {
        def.precipitation.kind = precipitation_kind::SNOW;
    }
    else {
        def.precipitation.kind = precipitation_kind::NONE;
    }

    const std::array fields = {
        std::make_pair("intensity", &def.precipitation.intensity),
        std::make_pair("speed", &def.precipitation.speed),
        std::make_pair("drift", &def.precipitation.drift),
        std::make_pair("size", &def.precipitation.size),
    };

    for(const auto& [field_name, field_ptr] : fields) {
        auto value = utils::opt_number(L, index, field_name, *field_ptr);

        if(!value.has_value()) {
            return fail(L, base);
        }

        field_ptr[0] = static_cast<float>(value.value());
    }

    if(!parse_color(L, index, "color", def.precipitation.color)) {
        return fail(L, base);
    }

    lua_settop(L, base);
    return true;
}

static bool parse_definition(lua_State* L, int def_idx, WeatherTypeDefinition& def, ModContext* ctx)
{
    const std::array numbers = {
        std::make_pair("intensity", &def.intensity),
        std::make_pair("sky_visibility", &def.sky_visibility),
        std::make_pair("light", &def.light_scale),
        std::make_pair("wind", &def.wind_speed),
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

    // The weight decides how big a slice of the type grid the type owns,
    // which is the only thing that makes one weather more common than another
    auto weight = utils::opt_integer(L, def_idx, "weight", static_cast<lua_Integer>(def.weight));

    if(!weight.has_value()) {
        return false;
    }

    if(weight.value() < 0) {
        lua_pushliteral(L, "weight must be non-negative");
        return false;
    }

    def.weight = static_cast<std::uint32_t>(weight.value());

    def.on_tick = parse_callback(L, def_idx, "on_tick", ctx);
    def.on_strike = parse_callback(L, def_idx, "on_strike", ctx);

    return parse_cloud(L, def_idx, def) && parse_precipitation(L, def_idx, def) && parse_color(L, def_idx, "sky_tint", def.sky_tint);
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

static int api_state(lua_State* L)
{
    auto& state = weather::state();

    lua_newtable(L);

    lua_pushnumber(L, static_cast<lua_Number>(state.types.scale));
    lua_setfield(L, -2, "types_scale");

    lua_pushinteger(L, static_cast<lua_Integer>(state.types.seed));
    lua_setfield(L, -2, "types_seed");

    lua_pushnumber(L, static_cast<lua_Number>(state.detail.scale));
    lua_setfield(L, -2, "detail_scale");

    lua_pushinteger(L, static_cast<lua_Integer>(state.detail.seed));
    lua_setfield(L, -2, "detail_seed");

    lua_pushnumber(L, static_cast<lua_Number>(state.wind.x()));
    lua_setfield(L, -2, "wind_x");

    lua_pushnumber(L, static_cast<lua_Number>(state.wind.y()));
    lua_setfield(L, -2, "wind_z");

    return 1;
}

static int api_sample(lua_State* L)
{
    auto x = luaL_checknumber(L, 1);
    auto y = luaL_checknumber(L, 2);
    auto z = luaL_checknumber(L, 3);

    (void)y;

    auto coverage = weather::coverage(static_cast<float>(x), static_cast<float>(z));
    auto type = weather::type_at(static_cast<float>(x), static_cast<float>(z));

    lua_newtable(L);

    lua_pushnumber(L, static_cast<lua_Number>(coverage));
    lua_setfield(L, -2, "coverage");

    lua_pushinteger(L, static_cast<lua_Integer>(type));
    lua_setfield(L, -2, "type");

    auto def = weather_registry::find_definition(type);

    if(def && def->precipitation.kind != precipitation_kind::NONE) {
        lua_pushnumber(L, static_cast<lua_Number>(def->precipitation.intensity * coverage));
    }
    else {
        lua_pushnumber(L, 0.0);
    }

    lua_setfield(L, -2, "precipitation");

    return 1;
}

static int api_force(lua_State* L)
{
    auto raw_name = luaL_checkstring(L, 1);
    auto type = weather_registry::resolve(raw_name);

    if(type == WEATHER_ID_NULL) {
        lua_pushfstring(L, "unknown weather: %s", raw_name);
        return lua_error(L);
    }

    auto x = luaL_optnumber(L, 2, 0.0);
    auto z = luaL_optnumber(L, 3, 0.0);

    weather::force_at(type, static_cast<float>(x), static_cast<float>(z));

    lua_pushboolean(L, true);
    return 1;
}

void api::open_weather_library(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // weather

    lua_pushinteger(L, static_cast<lua_Integer>(precipitation_kind::NONE));
    lua_setfield(L, -2, "PRECIP_NONE");
    lua_pushinteger(L, static_cast<lua_Integer>(precipitation_kind::RAIN));
    lua_setfield(L, -2, "PRECIP_RAIN");
    lua_pushinteger(L, static_cast<lua_Integer>(precipitation_kind::SNOW));
    lua_setfield(L, -2, "PRECIP_SNOW");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_pushcfunction(L, &api_state);
    lua_setfield(L, -2, "state");

    lua_pushcfunction(L, &api_sample);
    lua_setfield(L, -2, "sample");

    lua_pushcfunction(L, &api_force);
    lua_setfield(L, -2, "force");

    lua_setglobal(L, "weather");
}
