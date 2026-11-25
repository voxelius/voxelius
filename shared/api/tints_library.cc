#include "shared/pch.hh"

#include "shared/api/tints_library.hh"

#include "core/identifier.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/tint_registry.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_tint(candidate) == TINT_ID_NULL && tint_registry::find(candidate) == TINT_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto suffix = std::format("{}~{}", id.value(), n);
        auto candidate = Identifier::from_parts(id.name_space(), suffix);

        if(is_free(candidate)) {
            msg::log_warning("tints.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("tints.add: {}: out of resolvable names", id.full_string());
    return id;
}

static bool parse_definition(lua_State* L, int def_idx, TintDefinition& def, ModContext* ctx)
{
    auto default_color = utils::require_fvec<3>(L, def_idx, "default_color");

    if(!default_color.has_value()) {
        return false;
    }

    def.default_color = default_color->cast<float>();
    auto color = Eigen::Vector3f(def.default_color.cwiseMin(1.0f));
    def.default_color = color.cwiseMax(0.0f);

    return true;
}

static bool add_tint(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx, tint_id_type& out_tint_id)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "tints.add: malformed name: %s", raw_name);
        return false;
    }

    id = make_unique_id(id, ctx);

    TintDefinition def {};

    if(!parse_definition(L, def_idx, def, ctx)) {
        return false;
    }

    out_tint_id = ctx->register_tint(std::move(id), std::move(def));

    return true;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto raw_name = luaL_checkstring(L, 1);

    int def_idx = 2;

    luaL_checktype(L, def_idx, LUA_TTABLE);

    tint_id_type tint_id = TINT_ID_NULL;

    if(!add_tint(L, ctx, raw_name, def_idx, tint_id)) {
        return lua_error(L);
    }

    lua_pushinteger(L, tint_id);
    return 1;
}

void api_shared::open_tints(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L);

    lua_pushinteger(L, TINT_ID_NULL);
    lua_setfield(L, -2, "NULL_TINT");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, api_add, 1);
    lua_setfield(L, -2, "add");

    lua_setglobal(L, "tints");
}
