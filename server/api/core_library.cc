#include "server/pch.hh"

#include "server/api/core_library.hh"

#include "shared/api/core_library.hh"

void api_server::open_core(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    api_shared::open_core(lua, ctx);

    auto L = lua.get();

    lua_getglobal(L, "core");

    lua_pushboolean(L, false);
    lua_setfield(L, -2, "IS_CLIENT");

    lua_pushboolean(L, true);
    lua_setfield(L, -2, "IS_SERVER");

    lua_pop(L, 1);
}
