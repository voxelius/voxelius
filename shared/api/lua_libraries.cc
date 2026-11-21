#include "shared/pch.hh"

#include "shared/api/lua_libraries.hh"

#include "core/identifier.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"

#include "shared/mod_context.hh"

static bool load_dofile_chunk(lua_State* L, const ModContext* ctx, std::string_view path)
{
    auto identifier = Identifier::from_string(path, ctx->name_space());

    if(!identifier.is_valid()) {
        lua_pushstring(L, "malformed path: ");
        lua_pushlstring(L, path.data(), path.size());
        lua_concat(L, 2);
        return false;
    }

    std::string source;
    auto full_path = identifier.as_file_path("scripts", {});

    if(!utils::read_file(full_path, source)) {
        auto error_view = utils::physfs_error();
        lua_pushfstring(L, "%s: ", full_path.c_str());
        lua_pushlstring(L, error_view.data(), error_view.size());
        lua_concat(L, 2);
        return false;
    }

    auto chunk_name = std::format("@{}", full_path);
    auto load_status = luaL_loadbuffer(L, source.data(), source.size(), chunk_name.c_str());

    return load_status == LUA_OK;
}

static int api_override_dofile(lua_State* L)
{
    auto raw_path = luaL_checkstring(L, 1);
    auto path = std::string_view(raw_path);
    auto ctx = static_cast<const ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    if(!load_dofile_chunk(L, ctx, path)) {
        return lua_error(L);
    }

    auto stack_base = lua_gettop(L);
    auto pcall_status = lua_pcall(L, 0, LUA_MULTRET, 0);

    if(pcall_status != LUA_OK) {
        return lua_error(L);
    }

    return lua_gettop(L) - stack_base;
}

static bool do_require(lua_State* L, std::string_view path)
{
    auto identifier = Identifier::from_string(path);

    if(!identifier.is_valid() || utils::is_whitespace(identifier.name_space())) {
        lua_pushstring(L, "malformed path: ");
        lua_pushlstring(L, path.data(), path.size());
        lua_concat(L, 2);
        return false;
    }

    lua_pushstring(L, "VX_REQUIRE_CACHE");
    lua_rawget(L, LUA_REGISTRYINDEX);

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushstring(L, "VX_REQUIRE_CACHE");
        lua_pushvalue(L, -2);
        lua_rawset(L, LUA_REGISTRYINDEX);
    }

    lua_pushvalue(L, 1);
    lua_rawget(L, -2);

    if(!lua_isnil(L, -1)) {
        return true; // Cache hit
    }

    lua_pop(L, 1);

    std::string source;
    auto full_path = identifier.as_file_path("lib", ".lua");

    if(!utils::read_file(full_path, source)) {
        auto error_view = utils::physfs_error();
        lua_pushfstring(L, "%s: ", full_path.c_str());
        lua_pushlstring(L, error_view.data(), error_view.size());
        lua_concat(L, 2);
        return false;
    }

    auto chunk_name = std::format("@{}", full_path);
    auto load_status = luaL_loadbuffer(L, source.data(), source.size(), chunk_name.c_str());

    if(load_status != LUA_OK) {
        return false;
    }

    auto pcall_status = lua_pcall(L, 0, 1, 0);

    if(pcall_status != LUA_OK) {
        return false;
    }

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }

    lua_pushvalue(L, 1);
    lua_pushvalue(L, -2);
    lua_rawset(L, 2);

    return true;
}

static int api_override_require(lua_State* L)
{
    auto raw_path = luaL_checkstring(L, 1);
    auto path = std::string_view(raw_path);

    if(!do_require(L, path)) {
        return lua_error(L);
    }

    return 1;
}

static int api_override_print(lua_State* L)
{
    std::string message;
    message.reserve(512);

    auto argc = lua_gettop(L);

    for(int i = 1; i <= argc; ++i) {
        std::size_t length = 0;
        auto string = luaL_tolstring(L, i, &length);

        if(i > 1)
            message.push_back(char(0x09));
        message.append(string, length);

        lua_pop(L, 1);
    }

    const char* debug_file = "unknown";
    unsigned long debug_line = 0;
    lua_Debug ar;

    if(lua_getstack(L, 1, &ar) && lua_getinfo(L, "Sl", &ar)) {
        debug_file = ar.short_src;
        debug_line = std::max(0, ar.currentline);
    }

    msg::log_info("{}:{}: {}", debug_file, debug_line, message);

    return 0;
}

void api::open_lua_libraries(std::shared_ptr<lua_State>& lua, const ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    luaL_requiref(L, LUA_GNAME, &luaopen_base, 1);
    luaL_requiref(L, LUA_TABLIBNAME, &luaopen_table, 1);
    luaL_requiref(L, LUA_STRLIBNAME, &luaopen_string, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, &luaopen_math, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, &luaopen_utf8, 1);
    lua_pop(L, 5);

    lua_pushnil(L);
    lua_setglobal(L, "loadfile");

    lua_pushnil(L);
    lua_setglobal(L, "load");

    lua_pushnil(L);
    lua_setglobal(L, "collectgarbage");

    lua_pushlightuserdata(L, const_cast<ModContext*>(ctx));
    lua_pushcclosure(L, &api_override_dofile, 1);
    lua_setglobal(L, "dofile");

    lua_pushlightuserdata(L, const_cast<ModContext*>(ctx));
    lua_pushcclosure(L, &api_override_require, 1);
    lua_setglobal(L, "require");

    lua_pushlightuserdata(L, const_cast<ModContext*>(ctx));
    lua_pushcclosure(L, &api_override_print, 1);
    lua_setglobal(L, "print");
}
