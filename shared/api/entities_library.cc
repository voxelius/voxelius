#include "shared/pch.hh"

#include "shared/api/entities_library.hh"

#include "core/identifier.hh"

#include "shared/entity/class.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/entity/required_class.hh"
#include "shared/mod_context.hh"
#include "shared/utils/entity.hh"
#include "shared/utils/lua.hh"
#include "shared/world/world.hh"
#include "shared/world/world_scope.hh"

enum patch_result {
    PATCH_REJECT = 0,
    PATCH_INVALID,
    PATCH_FAILED,
    PATCH_OK
};

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_class(candidate) == CLASS_ID_NULL && class_registry::find(candidate) == CLASS_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto suffix = std::format("{}~{}", id.value(), n);
        auto candidate = Identifier::from_parts(id.name_space(), suffix);

        if(is_free(candidate)) {
            msg::log_warning("entities.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("entities.add: {}: out of resolvable names", id.full_string());
    return id;
}

static bool parse_component(lua_State* L, int, ModContext*, ClassDefinition_Entry& entry)
{
    auto component_name = utils::require_string(L, -2);

    if(!component_name.has_value()) {
        return false;
    }

    auto name = std::string(component_name.value());
    auto id = component_map::from_name(name);

    if(id == COMPONENT_ID_NULL) {
        lua_pushfstring(L, "entities.add: %s: unknown component", name.c_str());
        return false;
    }

    lua_pushvalue(L, -1); // Keep the config table: luaL_ref pops its argument

    auto config_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, config_ref);

    auto config = component_map::prepare(id, L, lua_gettop(L));

    lua_pop(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, config_ref);

    if(!config.has_value()) {
        return false;
    }

    entry.id = id;
    entry.name = name;
    entry.config = std::move(config);

    return true;
}

static bool parse_components(lua_State* L, int def_idx, ModContext* ctx, ClassDefinition& def)
{
    vx::hash_map<std::string, ClassDefinition_Entry> components;

    lua_pushnil(L);

    while(lua_next(L, def_idx)) {
        if(!lua_istable(L, -1)) {
            lua_pushfstring(L, "entities.add: config must be a table");
            return false;
        }

        ClassDefinition_Entry entry {};

        if(!parse_component(L, -1, ctx, entry)) {
            return false;
        }

        if(components.contains(entry.name)) {
            lua_pushfstring(L, "entities.add: duplicate component: %s", entry.name.c_str());
            return false;
        }

        components.insert_or_assign(entry.name, std::move(entry));

        lua_pop(L, 1);
    }

    def.entries.clear();
    def.entries.reserve(components.size());

    for(auto& it : components) {
        def.entries.emplace_back(std::move(it.second));
    }

    return true;
}

static std::optional<class_id_type> add_class(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "entities.add: malformed name: %s", raw_name);
        return std::nullopt;
    }

    id = make_unique_id(id, ctx);

    ClassDefinition def {};

    if(!parse_components(L, def_idx, ctx, def)) {
        return std::nullopt;
    }

    auto class_id = ctx->register_class(id, std::move(def));
    return class_id;
}

static int api_get(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto raw_name = luaL_checkstring(L, 1);
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushinteger(L, CLASS_ID_NULL);
        return 1;
    }

    auto local_id = ctx->find_class(id);

    if(local_id == CLASS_ID_NULL) {
        local_id = class_registry::find(id);
    }

    lua_pushinteger(L, local_id);
    return 1;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto raw_name = luaL_checkstring(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);

    auto class_id = add_class(L, ctx, raw_name, 2);

    if(!class_id.has_value()) {
        return lua_error(L);
    }

    lua_pushinteger(L, class_id.value());
    return 1;
}

static std::optional<entt::entity> spawn_entity(lua_State* L, ModContext* ctx, std::string_view raw_name, int kv_idx)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    auto& registry = world->registry();

    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushliteral(L, "entities.spawn: malformed name: ");
        lua_pushlstring(L, raw_name.data(), raw_name.size());
        lua_concat(L, 2);

        return std::nullopt;
    }

    auto entity = utils::spawn(registry, id, L, kv_idx);

    if(!registry.valid(entity)) {
        auto id_string = id.full_string();

        lua_pushliteral(L, "entities.spawn: ");
        lua_pushlstring(L, id_string.data(), id_string.size());
        lua_pushliteral(L, ": unable to spawn");
        lua_concat(L, 3);

        return std::nullopt;
    }

    return entity;
}

static bool spawn(lua_State* L, ModContext* ctx)
{
    auto raw_name = utils::require_string(L, 1);

    if(!raw_name.has_value()) {
        return false;
    }

    int kv_idx;

    if(lua_gettop(L) >= 2) {
        if(!lua_istable(L, 2)) {
            lua_pushliteral(L, "entities.spawn: config must be a table");
            return false;
        }

        kv_idx = 2;
    }
    else {
        lua_newtable(L);
        kv_idx = lua_gettop(L);
    }

    auto entity = spawn_entity(L, ctx, raw_name.value(), kv_idx);

    if(!entity.has_value()) {
        return false;
    }

    lua_pushinteger(L, static_cast<lua_Integer>(entity.value()));
    return true;
}

static int api_spawn(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    if(!spawn(L, ctx)) {
        return lua_error(L);
    }

    return 1;
}

static bool do_set_player(lua_State* L, ModContext* ctx, const char* raw_name)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "entities.set_player: malformed name: %s", raw_name);
        return false;
    }

    required_class::set_player(id, ctx);

    return true;
}

static int api_set_player(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto raw_name = luaL_checkstring(L, 1);

    if(!do_set_player(L, ctx, raw_name)) {
        return lua_error(L);
    }

    return 0;
}

static patch_result patch(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return PATCH_REJECT;
    }

    auto& registry = world->registry();

    auto raw_entity = luaL_checkinteger(L, 1);
    auto entity = static_cast<entt::entity>(raw_entity);

    if(!registry.valid(entity)) {
        return PATCH_INVALID;
    }

    auto component_name = utils::require_string(L, 2);

    if(!component_name.has_value()) {
        return PATCH_REJECT;
    }

    auto id = component_map::from_name(component_name.value());

    if(id == COMPONENT_ID_NULL) {
        lua_pushliteral(L, "entities.patch: ");
        lua_pushlstring(L, component_name->data(), component_name->size());
        lua_pushliteral(L, ": unknown component");
        lua_concat(L, 3);

        return PATCH_REJECT;
    }

    if(!lua_istable(L, 3)) {
        lua_pushliteral(L, "entities.patch: config must be a table");
        return PATCH_REJECT;
    }

    if(!component_map::patch(registry, id, entity, L, 3)) {
        return PATCH_FAILED;
    }

    return PATCH_OK;
}

static int api_patch(lua_State* L)
{
    switch(patch(L)) {
        case PATCH_REJECT:
            return lua_error(L);
        case PATCH_INVALID:
            lua_pushboolean(L, false);
            return 1;
        case PATCH_FAILED:
            lua_pushboolean(L, false);
            lua_insert(L, -2); // False before the error message already on the stack
            return 2;
        case PATCH_OK:
            lua_pushboolean(L, true);
            return 1;
    }

    return 0;
}

static std::optional<bool> despawn(lua_State* L)
{
    auto world = WorldScope::active();

    if(world == nullptr) {
        lua_pushliteral(L, "no active world");
        return std::nullopt;
    }

    auto& registry = world->registry();

    auto raw_entity = luaL_checkinteger(L, 1);
    auto entity = static_cast<entt::entity>(raw_entity);

    if(!registry.valid(entity)) {
        return false;
    }

    registry.destroy(entity);
    return true;
}

static int api_despawn(lua_State* L)
{
    auto value = despawn(L);

    if(!value.has_value()) {
        return lua_error(L);
    }

    lua_pushboolean(L, value.value());
    return 1;
}

void api_shared::open_entities(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // Entities

    lua_pushinteger(L, CLASS_ID_NULL);
    lua_setfield(L, -2, "NULL_CLASS");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_get, 1);
    lua_setfield(L, -2, "get");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_spawn, 1);
    lua_setfield(L, -2, "spawn");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_set_player, 1);
    lua_setfield(L, -2, "set_player");

    lua_pushcfunction(L, api_patch);
    lua_setfield(L, -2, "patch");

    lua_pushcfunction(L, api_despawn);
    lua_setfield(L, -2, "despawn");

    lua_setglobal(L, "entities");
}
