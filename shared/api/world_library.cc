#include "shared/pch.hh"

#include "shared/api/world_library.hh"

#include "shared/globals.hh"
#include "shared/utils/lua.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

static int api_get_block(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto block_id = globals::world->get_block(pos);
    auto family = block_registry::find_family_of(block_id);

    if(family == nullptr) {
        lua_pushinteger(L, static_cast<lua_Integer>(block_id));
        return 1;
    }
    else {
        lua_pushinteger(L, static_cast<lua_Integer>(family->stem_id));
        return 1;
    }
}

static int api_set_block(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto block_id = static_cast<block_id_type>(luaL_checkinteger(L, 4));
    auto success = globals::world->set_block(pos, block_id);

    lua_pushboolean(L, success);
    return 1;
}

static int api_get_fluid(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto fluid = globals::world->get_fluid(pos);

    lua_pushinteger(L, static_cast<lua_Integer>(fluid));
    return 1;
}

static int api_get_fluid_faces(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto faces = globals::world->get_fluid_faces(pos);

    lua_pushinteger(L, static_cast<lua_Integer>(faces));
    return 1;
}

static int api_get_light(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto light = globals::world->get_light(pos);

    lua_pushinteger(L, static_cast<lua_Integer>(light));
    return 1;
}

static int api_get_state(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto state_name = luaL_checkstring(L, 4);
    auto value_opt = globals::world->get_state(pos, state_name);

    if(value_opt.has_value()) {
        lua_pushlstring(L, value_opt->data(), value_opt->size());
        return 1;
    }
    else {
        lua_pushnil(L);
        return 1;
    }
}

static int api_set_state(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto state_name = luaL_checkstring(L, 4);
    auto state_value = luaL_checkstring(L, 5);

    auto success = globals::world->set_state(pos, state_name, state_value);

    lua_pushboolean(L, success);
    return 1;
}

static int api_get_state_fluid(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto state_name = luaL_checkstring(L, 4);
    auto state_value = luaL_checkstring(L, 5);
    auto fluid = globals::world->get_state_fluid(pos, state_name, state_value);

    lua_pushinteger(L, static_cast<lua_Integer>(fluid));
    return 1;
}

static int api_get_temperature_base(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto temperature = globals::world->get_temperature_base(pos);

    lua_pushnumber(L, static_cast<lua_Number>(temperature));
    return 1;
}

static int api_get_temperature(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto temperature = globals::world->get_temperature(pos);

    lua_pushnumber(L, static_cast<lua_Number>(temperature));
    return 1;
}

static int api_get_humidity_base(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto humidity = globals::world->get_humidity_base(pos) * 100.0f;

    lua_pushnumber(L, static_cast<lua_Number>(humidity));
    return 1;
}

static int api_get_humidity(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto humidity = globals::world->get_humidity(pos) * 100.0f;

    lua_pushnumber(L, static_cast<lua_Number>(humidity));
    return 1;
}

static int api_schedule(lua_State* L)
{
    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto dt = static_cast<std::uint64_t>(luaL_checkinteger(L, 4));
    auto deadline = globals::current_tick + dt;

    globals::world->schedule(pos, deadline, BLOCK_TICK_SCRIPTED);

    return 0;
}

void api::open_world_library(std::shared_ptr<lua_State>& lua, const ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // world

    lua_pushcfunction(L, api_get_block);
    lua_setfield(L, -2, "get_block");

    lua_pushcfunction(L, api_get_fluid);
    lua_setfield(L, -2, "get_fluid");

    lua_pushcfunction(L, api_get_fluid_faces);
    lua_setfield(L, -2, "get_fluid_faces");

    lua_pushcfunction(L, api_set_block);
    lua_setfield(L, -2, "set_block");

    lua_pushcfunction(L, api_get_light);
    lua_setfield(L, -2, "get_light");

    lua_pushcfunction(L, api_get_state);
    lua_setfield(L, -2, "get_state");

    lua_pushcfunction(L, api_set_state);
    lua_setfield(L, -2, "set_state");

    lua_pushcfunction(L, api_get_state_fluid);
    lua_setfield(L, -2, "get_state_fluid");

    lua_pushcfunction(L, api_get_temperature_base);
    lua_setfield(L, -2, "get_temperature_base");

    lua_pushcfunction(L, api_get_temperature);
    lua_setfield(L, -2, "get_temperature");

    lua_pushcfunction(L, api_get_humidity_base);
    lua_setfield(L, -2, "get_humidity_base");

    lua_pushcfunction(L, api_get_humidity);
    lua_setfield(L, -2, "get_humidity");

    lua_pushcfunction(L, api_schedule);
    lua_setfield(L, -2, "schedule");

    lua_setglobal(L, "world");
}
