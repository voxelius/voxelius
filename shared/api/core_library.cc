#include "shared/pch.hh"

#include "shared/api/core_library.hh"

#include "core/version.hh"

#include "shared/mod_context.hh"

void api::open_core_library(std::shared_ptr<lua_State>& lua, const ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // Core

    lua_pushinteger(L, version::major);
    lua_setfield(L, -2, "VERSION_MAJOR");

    lua_pushinteger(L, version::minor);
    lua_setfield(L, -2, "VERSION_MINOR");

    lua_pushinteger(L, version::patch);
    lua_setfield(L, -2, "VERSION_PATCH");

    lua_pushlstring(L, version::scm_branch.data(), version::scm_branch.size());
    lua_setfield(L, -2, "VERSION_SCM_BRANCH");

    lua_pushlstring(L, version::scm_revision.data(), version::scm_revision.size());
    lua_setfield(L, -2, "VERSION_SCM_REVISION");

    lua_pushlstring(L, version::semantic.data(), version::semantic.size());
    lua_setfield(L, -2, "VERSION_SEMANTIC");

    lua_pushlstring(L, version::full.data(), version::full.size());
    lua_setfield(L, -2, "VERSION");

    auto name_space = ctx->name_space();
    lua_pushlstring(L, name_space.data(), name_space.size());
    lua_setfield(L, -2, "NAMESPACE");

    lua_setglobal(L, "core");
}
