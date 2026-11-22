#include "server/pch.hh"

#include "server/world/generator_settings.hh"

bool generator_settings::read_number(lua_State* L, int table_idx, const char* generator, const char* key, float& out)
{
    if(table_idx == 0) {
        return true;
    }

    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(lua_type(L, -1) != LUA_TNUMBER) {
        lua_pop(L, 1);
        lua_pushfstring(L, "%s: %s must be a number", generator, key);
        return false;
    }

    out = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    return true;
}

bool generator_settings::read_integer(lua_State* L, int table_idx, const char* generator, const char* key, lua_Integer& out)
{
    if(table_idx == 0) {
        return true;
    }

    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(lua_type(L, -1) != LUA_TNUMBER || !lua_isinteger(L, -1)) {
        lua_pop(L, 1);
        lua_pushfstring(L, "%s: %s must be an integer", generator, key);
        return false;
    }

    out = lua_tointeger(L, -1);
    lua_pop(L, 1);

    return true;
}

bool generator_settings::read_table(lua_State* L, int table_idx, const char* generator, const char* key, int& index)
{
    index = 0;

    if(table_idx == 0) {
        return true;
    }

    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushfstring(L, "%s: %s must be a table", generator, key);
        return false;
    }

    index = lua_gettop(L);

    return true;
}

bool generator_settings::check_keys(lua_State* L, int table_idx, const char* generator, std::span<const char* const> allowed)
{
    if(table_idx == 0) {
        return true;
    }

    lua_pushnil(L);

    while(lua_next(L, table_idx)) {
        if(lua_type(L, -2) != LUA_TSTRING) {
            lua_pop(L, 2);
            lua_pushfstring(L, "%s: setting keys must be strings", generator);
            return false;
        }

        auto key = std::string_view(lua_tostring(L, -2));

        auto known = std::any_of(allowed.begin(), allowed.end(), [&](const char* const item) {
            return key == item;
        });

        if(known) {
            lua_pop(L, 1);
            continue;
        }

        lua_pushfstring(L, "%s: unknown setting \"%s\"", generator, lua_tostring(L, -2));
        lua_remove(L, -2);
        lua_remove(L, -2);
        return false;
    }

    return true;
}
