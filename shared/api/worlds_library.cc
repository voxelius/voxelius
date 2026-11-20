#include "shared/pch.hh"

#include "shared/api/worlds_library.hh"

#include "core/identifier.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/world_registry.hh"

static bool fail(lua_State* L, int base, std::string_view message)
{
    lua_settop(L, base);
    lua_pushlstring(L, message.data(), message.size());
    return false;
}

static bool parse_name_list(lua_State* L, int base, int table_idx, const char* key, const ModContext* ctx, std::vector<Identifier>& out)
{
    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "name list must be a table");
    }

    auto count = static_cast<std::size_t>(lua_rawlen(L, -1));

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            lua_pop(L, 2);
            return fail(L, base, "name list entry must be a string");
        }

        out.push_back(Identifier::from_string(value.value(), ctx->name_space()));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto base = lua_gettop(L);

    auto raw_name = luaL_checkstring(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);

    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "worlds.add: malformed name: %s", raw_name);
        return lua_error(L);
    }

    auto definition = world_registry::add(id);

    if(definition == nullptr) {
        lua_pushfstring(L, "worlds.add: %s is already registered", raw_name);
        return lua_error(L);
    }

    auto generator = utils::opt_string(L, 2, "generator", {});

    if(!generator.has_value()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: invalid generator", raw_name);
        return lua_error(L);
    }

    if(generator->empty()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: generator is required", raw_name);
        return lua_error(L);
    }

    definition->generator = std::string(generator.value());

    auto directory = utils::opt_string(L, 2, "directory", {});

    if(!directory.has_value()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: invalid directory", raw_name);
        return lua_error(L);
    }

    definition->directory = std::string(directory.value());

    auto min_y = utils::opt_integer(L, 2, "min_y", -64);

    if(!min_y.has_value()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: invalid min_y", raw_name);
        return lua_error(L);
    }

    definition->min_y = static_cast<BlockPos::value_type>(min_y.value());

    auto max_y = utils::opt_integer(L, 2, "max_y", 255);

    if(!max_y.has_value()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: invalid max_y", raw_name);
        return lua_error(L);
    }

    definition->max_y = static_cast<BlockPos::value_type>(max_y.value());

    auto spawn = utils::opt_ivec<3>(L, 2, "spawn", Eigen::Vector<lua_Integer, 3>(0, 32, 0));

    if(!spawn.has_value()) {
        world_registry::remove(id);
        lua_pushfstring(L, "worlds.add: %s: invalid spawn", raw_name);
        return lua_error(L);
    }

    definition->spawn = BlockPos(spawn->x(), spawn->y(), spawn->z());

    if(!parse_name_list(L, base, 2, "biomes", ctx, definition->biome_names)) {
        world_registry::remove(id);
        return lua_error(L);
    }

    if(!parse_name_list(L, base, 2, "weather", ctx, definition->weather_names)) {
        world_registry::remove(id);
        return lua_error(L);
    }

    return 0;
}

void api::open_worlds_library(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // worlds

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_setglobal(L, "worlds");
}
