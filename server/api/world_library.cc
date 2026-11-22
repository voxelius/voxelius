#include "server/pch.hh"

#include "server/api/world_library.hh"

#include "shared/api/world_library.hh"
#include "shared/coord.hh"
#include "shared/globals.hh"
#include "shared/world/block.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

#include "server/world/world.hh"

static bool schedule(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return false;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto dt = static_cast<std::uint64_t>(luaL_checkinteger(L, 4));
    auto deadline = globals::current_tick + dt;

    // The server is the only side that opens this library, and every
    // server world is a ServerWorld
    auto server_world = static_cast<ServerWorld*>(world);

    server_world->schedule(pos, deadline, BLOCK_TICK_SCRIPTED);

    return true;
}

static int api_schedule(lua_State* L)
{
    if(!schedule(L)) {
        return lua_error(L);
    }

    return 0;
}

static std::optional<std::uint64_t> get_seed(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    return world->seed;
}

static int api_get_seed(lua_State* L)
{
    auto value = get_seed(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

void api_server::open_world(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    api_shared::open_world(lua, ctx);

    auto L = lua.get();

    lua_getglobal(L, "world");

    lua_pushcfunction(L, api_schedule);
    lua_setfield(L, -2, "schedule");

    lua_pushcfunction(L, api_get_seed);
    lua_setfield(L, -2, "seed");

    lua_pop(L, 1);
}
