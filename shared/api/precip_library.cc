#include "shared/pch.hh"

#include "shared/api/precip_library.hh"

#include "core/identifier.hh"
#include "core/msg/log.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/precip.hh"
#include "shared/world/precip_registry.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        auto result = true;
        result = result && ctx->find_precip(candidate) == PRECIP_ID_NULL;
        result = result && precip_registry::find(candidate) == PRECIP_ID_NULL;
        return result;
    };

    if(is_free(id)) {
        return id;
    }

    auto id_value = id.value();
    auto name_space = id.name_space();

    for(unsigned n = 1; n < 10000; ++n) {
        auto suffix = std::format("{}~{}", id_value, n);
        auto candidate = Identifier::from_parts(name_space, suffix);

        if(is_free(candidate)) {
            auto id_text = id.full_string();
            auto candidate_text = candidate.full_string();
            msg::log_warning("precip.add: {} is already registered, using {} instead", id_text, candidate_text);
            return candidate;
        }
    }

    auto id_text = id.full_string();
    msg::log_critical("precip.add: {}: out of resolvable names", id_text);
    return id;
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

// A band is a soft interval on one climate axis: the weight is one inside
// it and falls to zero over `fade` outside it. A missing table leaves the
// default band open on both sides, so a type that names no band always fits
static bool parse_band(lua_State* L, int table_idx, const char* key, float& min, float& max, float& fade)
{
    auto base = lua_gettop(L);

    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_settop(L, base);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, std::format("{} must be a table", key));
    }

    auto index = lua_gettop(L);

    auto parsed_min = utils::opt_number(L, index, "min", min);

    if(!parsed_min.has_value()) {
        return fail(L, base);
    }

    auto parsed_max = utils::opt_number(L, index, "max", max);

    if(!parsed_max.has_value()) {
        return fail(L, base);
    }

    auto parsed_fade = utils::opt_number(L, index, "fade", fade);

    if(!parsed_fade.has_value()) {
        return fail(L, base);
    }

    min = static_cast<float>(parsed_min.value());
    max = static_cast<float>(parsed_max.value());
    fade = static_cast<float>(parsed_fade.value());

    if(fade < 0.0f) {
        return fail(L, base, std::format("{}: fade must not be negative", key));
    }

    if(min > max) {
        return fail(L, base, std::format("{}: min must not exceed max", key));
    }

    lua_settop(L, base);
    return true;
}

static bool parse_identifier(lua_State* L, int table_idx, const char* key, const ModContext* ctx, Identifier& out)
{
    auto base = lua_gettop(L);

    auto value = utils::opt_string(L, table_idx, key, "");

    if(!value.has_value()) {
        return fail(L, base);
    }

    if(value->empty()) {
        out = {};
        return true;
    }

    auto id = Identifier::from_string(value.value(), ctx->name_space());

    if(!id.is_valid()) {
        return fail(L, base, std::format("{}: malformed identifier", key));
    }

    out = id;
    return true;
}

static bool parse_definition(lua_State* L, int def_idx, PrecipDefinition& def, const ModContext* ctx)
{
    auto base = lua_gettop(L);

    if(!parse_band(L, def_idx, "temperature", def.temperature_min, def.temperature_max, def.temperature_fade)) {
        return false;
    }

    if(!parse_band(L, def_idx, "humidity", def.humidity_min, def.humidity_max, def.humidity_fade)) {
        return false;
    }

    auto priority = utils::opt_number(L, def_idx, "priority", def.priority);

    if(!priority.has_value()) {
        return fail(L, base);
    }

    if(priority.value() <= 0.0) {
        return fail(L, base, "priority must be positive");
    }

    def.priority = static_cast<float>(priority.value());

    const std::array numbers = {
        std::make_pair("width", &def.quad_width),
        std::make_pair("height", &def.band_half_height),
        std::make_pair("fall", &def.fall_speed),
        std::make_pair("density", &def.draw_chance),
        std::make_pair("opacity", &def.opacity),
        std::make_pair("light_boost", &def.light_boost),
    };

    for(const auto& [field_name, field_ptr] : numbers) {
        auto value = utils::opt_number(L, def_idx, field_name, *field_ptr);

        if(!value.has_value()) {
            return fail(L, base);
        }

        field_ptr[0] = static_cast<float>(value.value());
    }

    if(def.quad_width <= 0.0f) {
        return fail(L, base, "width must be positive");
    }

    if(def.band_half_height <= 0.0f) {
        return fail(L, base, "height must be positive");
    }

    if(def.draw_chance < 0.0f) {
        return fail(L, base, "density must not be negative");
    }

    if(def.opacity < 0.0f || def.opacity > 1.0f) {
        return fail(L, base, "opacity must be in 0 to 1");
    }

    if(def.light_boost < 0.0f || def.light_boost > 1.0f) {
        return fail(L, base, "light_boost must be in 0 to 1");
    }

    if(!parse_identifier(L, def_idx, "texture", ctx, def.texture)) {
        return false;
    }

    if(!parse_identifier(L, def_idx, "tint", ctx, def.tint)) {
        return false;
    }

    if(!parse_color(L, def_idx, "color", def.color)) {
        return fail(L, base);
    }

    return true;
}

static std::optional<precip_type_id> add_definition(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "precip.add: malformed name: %s", raw_name);
        return std::nullopt;
    }

    id = make_unique_id(id, ctx);

    PrecipDefinition def {};
    def.name = id;

    if(!parse_definition(L, def_idx, def, ctx)) {
        return std::nullopt;
    }

    return ctx->register_precip(id, std::move(def));
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

static std::optional<precip_type_id> get(lua_State* L)
{
    auto raw_name = utils::require_string(L, 1);

    if(!raw_name.has_value()) {
        return std::nullopt;
    }

    return precip_registry::resolve(raw_name.value());
}

static int api_get(lua_State* L)
{
    auto id = get(L);

    if(!id.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(id.value()));
    return 1;
}

void api_shared::open_precip(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L);

    lua_pushinteger(L, static_cast<lua_Integer>(PRECIP_ID_NULL));
    lua_setfield(L, -2, "NULL_TYPE");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_pushcfunction(L, &api_get);
    lua_setfield(L, -2, "get");

    lua_setglobal(L, "precip");
}
