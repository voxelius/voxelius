#include "server/pch.hh"

#include "server/api/weather_library.hh"

#include "shared/api/weather_library.hh"
#include "shared/utils/lua.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

static std::optional<bool> force(lua_State* L)
{
    auto raw_name = utils::require_string(L, 1);

    if(!raw_name.has_value()) {
        return std::nullopt;
    }

    auto type = weather_registry::resolve(raw_name.value());

    if(type == WEATHER_ID_NULL) {
        lua_pushliteral(L, "unknown weather: ");
        lua_pushlstring(L, raw_name->data(), raw_name->size());
        lua_concat(L, 2);

        return std::nullopt;
    }

    auto x = utils::opt_number(L, 2, 0.0);

    if(!x.has_value()) {
        return std::nullopt;
    }

    auto z = utils::opt_number(L, 3, 0.0);

    if(!z.has_value()) {
        return std::nullopt;
    }

    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    auto& definition = world->definition();
    auto& weather_state = world->weather();

    return weather_state.force_at(definition.weather, type, static_cast<float>(x.value()), static_cast<float>(z.value()));
}

static int api_force(lua_State* L)
{
    auto value = force(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushboolean(L, value.value());
    return 1;
}

void api_server::open_weather(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    api_shared::open_weather(lua, ctx);

    auto L = lua.get();

    lua_getglobal(L, "weather");

    lua_pushcfunction(L, api_force);
    lua_setfield(L, -2, "force");

    lua_pop(L, 1);
}
