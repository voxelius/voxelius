#include "shared/pch.hh"

#include "shared/api/world_library.hh"

#include "shared/utils/lua.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

static std::optional<block_id_type> get_block(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto block_id = world->get_block(pos);
    auto family = block_registry::find_family_of(block_id);

    if(family) {
        return family->stem_id;
    }

    return block_id;
}

static int api_get_block(lua_State* L)
{
    auto value = get_block(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

static std::optional<bool> set_block(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto block_id = static_cast<block_id_type>(luaL_checkinteger(L, 4));

    return world->set_block(pos, block_id);
}

static int api_set_block(lua_State* L)
{
    auto value = set_block(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushboolean(L, value.value());
    return 1;
}

static std::optional<fluid_id_type> get_fluid(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_fluid(pos);
}

static int api_get_fluid(lua_State* L)
{
    auto value = get_fluid(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

static std::optional<std::uint32_t> get_fluid_faces(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_fluid_faces(pos);
}

static int api_get_fluid_faces(lua_State* L)
{
    auto value = get_fluid_faces(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

static std::optional<block_light_type> get_light(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_light(pos);
}

static int api_get_light(lua_State* L)
{
    auto value = get_light(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

static bool get_state(lua_State* L)
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

    auto state_name = utils::require_string(L, 4);

    if(!state_name.has_value()) {
        return false;
    }

    auto value_opt = world->get_state(pos, state_name.value());

    if(value_opt.has_value()) {
        lua_pushlstring(L, value_opt->data(), value_opt->size());
    }
    else {
        lua_pushnil(L);
    }

    return true;
}

static int api_get_state(lua_State* L)
{
    if(!get_state(L)) {
        return lua_error(L);
    }

    return 1;
}

static std::optional<bool> set_state(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto state_name = utils::require_string(L, 4);

    if(!state_name.has_value()) {
        return std::nullopt;
    }

    auto state_value = utils::require_string(L, 5);

    if(!state_value.has_value()) {
        return std::nullopt;
    }

    return world->set_state(pos, state_name.value(), state_value.value());
}

static int api_set_state(lua_State* L)
{
    auto value = set_state(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushboolean(L, value.value());
    return 1;
}

static std::optional<fluid_id_type> get_state_fluid(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    auto state_name = utils::require_string(L, 4);

    if(!state_name.has_value()) {
        return std::nullopt;
    }

    auto state_value = utils::require_string(L, 5);

    if(!state_value.has_value()) {
        return std::nullopt;
    }

    return world->get_state_fluid(pos, state_name.value(), state_value.value());
}

static int api_get_state_fluid(lua_State* L)
{
    auto value = get_state_fluid(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, static_cast<lua_Integer>(value.value()));
    return 1;
}

static std::optional<float> get_temperature_base(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_temperature_base(pos);
}

static int api_get_temperature_base(lua_State* L)
{
    auto value = get_temperature_base(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushnumber(L, static_cast<lua_Number>(value.value()));
    return 1;
}

static std::optional<float> get_temperature(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_temperature(pos);
}

static int api_get_temperature(lua_State* L)
{
    auto value = get_temperature(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushnumber(L, static_cast<lua_Number>(value.value()));
    return 1;
}

static std::optional<float> get_humidity_base(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_humidity_base(pos) * 100.0f;
}

static int api_get_humidity_base(lua_State* L)
{
    auto value = get_humidity_base(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushnumber(L, static_cast<lua_Number>(value.value()));
    return 1;
}

static std::optional<float> get_humidity(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    BlockPos pos = BlockPos::Zero();
    pos.x() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 1));
    pos.y() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 2));
    pos.z() = static_cast<BlockPos::value_type>(luaL_checkinteger(L, 3));

    return world->get_humidity(pos) * 100.0f;
}

static int api_get_humidity(lua_State* L)
{
    auto value = get_humidity(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushnumber(L, static_cast<lua_Number>(value.value()));
    return 1;
}

static std::optional<std::string_view> get_id(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    auto& identifier = world->id();
    return identifier.full_string();
}

static int api_get_id(lua_State* L)
{
    auto value = get_id(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushlstring(L, value->data(), value->size());
    return 1;
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

void api_shared::open_world(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // World

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

    lua_pushcfunction(L, api_get_id);
    lua_setfield(L, -2, "id");

    lua_pushcfunction(L, api_get_seed);
    lua_setfield(L, -2, "seed");

    lua_setglobal(L, "world");
}
