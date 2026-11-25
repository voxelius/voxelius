#include "shared/pch.hh"

#include "shared/api/fluids_library.hh"

#include "core/identifier.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/fluid_registry.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_fluid(candidate) == FLUID_ID_NULL && fluid_registry::find(candidate) == FLUID_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto suffix = std::format("{}~{}", id.value(), n);
        auto candidate = Identifier::from_parts(id.name_space(), suffix);

        if(is_free(candidate)) {
            msg::log_warning("fluids.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("fluids.add: {}: out of resolvable names", id.full_string());
    return id;
}

static bool parse_textures(lua_State* L, int idx, std::vector<Identifier>& out, ModContext* ctx)
{
    if(lua_isnil(L, idx)) {
        out.clear();
        return true;
    }

    if(!lua_istable(L, idx)) {
        auto type_name = lua_typename(L, lua_type(L, idx));
        lua_pushfstring(L, "expected a table, got %s", type_name);
        return false;
    }

    out.clear();

    lua_pushnil(L);

    while(lua_next(L, idx)) {
        auto value_str = utils::require_string(L, -1);

        if(!value_str.has_value()) {
            return false;
        }

        auto id = Identifier::from_string(value_str.value(), ctx->name_space());

        if(!id.is_valid()) {
            lua_pushfstring(L, "malformed texture name: %s", value_str.value());
            return false;
        }

        out.push_back(std::move(id));

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_physics(lua_State* L, int idx, FluidPhysics& out)
{
    if(!lua_istable(L, idx)) {
        auto type_name = lua_typename(L, lua_type(L, idx));
        lua_pushfstring(L, "expected a physics table, got %s", type_name);
        return false;
    }

    auto drag = utils::require_number(L, idx, "drag");

    if(!drag.has_value()) {
        return false;
    }

    auto gravity_scale = utils::require_number(L, idx, "gravity_scale");

    if(!gravity_scale.has_value()) {
        return false;
    }

    auto input_scale = utils::require_number(L, idx, "input_scale");

    if(!input_scale.has_value()) {
        return false;
    }

    auto swim_speed = utils::require_number(L, idx, "swim_speed");

    if(!swim_speed.has_value()) {
        return false;
    }

    auto climb_speed = utils::require_number(L, idx, "climb_speed");

    if(!climb_speed.has_value()) {
        return false;
    }

    out.drag = static_cast<float>(drag.value());
    out.gravity_scale = static_cast<float>(gravity_scale.value());
    out.input_scale = static_cast<float>(input_scale.value());
    out.swim_speed = static_cast<float>(swim_speed.value());
    out.climb_speed = static_cast<float>(climb_speed.value());

    return true;
}

static bool parse_definition(lua_State* L, int def_idx, FluidDefinition& def, ModContext* ctx)
{
    auto gravity = utils::require_integer(L, def_idx, "gravity");

    if(!gravity.has_value()) {
        return false;
    }

    def.gravity = static_cast<fluid_gravity>(gravity.value());

    auto full_level = utils::require_integer(L, def_idx, "full_level");

    if(!full_level.has_value()) {
        return false;
    }

    def.full_level = static_cast<unsigned>(full_level.value());
    def.full_level = std::clamp(def.full_level, 1U, 15U);

    lua_getfield(L, def_idx, "physics");

    auto physics_idx = lua_gettop(L);

    if(!parse_physics(L, physics_idx, def.physics)) {
        return false;
    }

    lua_pop(L, 1);

    lua_getfield(L, def_idx, "opaque");

    if(!lua_isnil(L, -1)) {
        if(!lua_isboolean(L, -1)) {
            auto type_name = lua_typename(L, lua_type(L, -1));
            lua_pushfstring(L, "expected a boolean, got %s", type_name);
            return false;
        }

        def.opaque = lua_toboolean(L, -1);
    }

    lua_pop(L, 1);

    auto fog_density = utils::opt_number(L, def_idx, "fog_density", 1.0);

    if(!fog_density.has_value()) {
        return false;
    }

    def.fog_density = std::max(static_cast<float>(fog_density.value()), 1.0f);

    auto fog_color = utils::opt_fvec<3>(L, def_idx, "fog_color", def.fog_color.cast<double>());

    if(!fog_color.has_value()) {
        return false;
    }

    def.fog_color = fog_color->cast<float>();
    auto color = Eigen::Vector3f(def.fog_color.cwiseMax(0.0f));
    def.fog_color = color.cwiseMin(1.0f);

    auto tint_name = utils::opt_string(L, def_idx, "tint", {});

    if(!tint_name.has_value()) {
        return false;
    }

    if(tint_name->size()) {
        def.tint_name = Identifier::from_string(tint_name.value(), ctx->name_space());
    }

    lua_getfield(L, def_idx, "albedo");

    if(lua_isnil(L, -1)) {
        lua_pushfstring(L, "missing required albedo field");
        return false;
    }

    if(!lua_istable(L, -1)) {
        auto type_name = lua_typename(L, lua_type(L, -1));
        lua_pushfstring(L, "expected a table, got %s", type_name);
        return false;
    }

    lua_getfield(L, -1, "still");

    auto still_idx = lua_gettop(L);

    if(!parse_textures(L, still_idx, def.albedo_still, ctx)) {
        return false;
    }

    lua_pop(L, 1);

    if(def.albedo_still.empty()) {
        lua_pushfstring(L, "missing required still textures");
        return false;
    }

    lua_getfield(L, -1, "flowing");

    auto flowing_idx = lua_gettop(L);

    if(!parse_textures(L, flowing_idx, def.albedo_flowing, ctx)) {
        return false;
    }

    lua_pop(L, 1);

    if(def.albedo_flowing.empty()) {
        lua_pushfstring(L, "missing required flowing textures");
        return false;
    }

    lua_pop(L, 1);

    lua_getfield(L, def_idx, "masks");

    if(!lua_isnil(L, -1)) {
        if(!lua_istable(L, -1)) {
            auto type_name = lua_typename(L, lua_type(L, -1));
            lua_pushfstring(L, "expected a table, got %s", type_name);
            return false;
        }

        auto masks_idx = lua_gettop(L);

        auto still_mask = utils::opt_string(L, masks_idx, "still", {});

        if(!still_mask.has_value()) {
            return false;
        }

        if(still_mask->size()) {
            def.mask_still = Identifier::from_string(still_mask.value(), ctx->name_space());

            if(!def.mask_still->is_valid()) {
                lua_pushfstring(L, "malformed still mask name: %s", still_mask.value());
                return false;
            }
        }

        auto flowing_mask = utils::opt_string(L, masks_idx, "flowing", {});

        if(!flowing_mask.has_value()) {
            return false;
        }

        if(flowing_mask->size()) {
            def.mask_flowing = Identifier::from_string(flowing_mask.value(), ctx->name_space());

            if(!def.mask_flowing->is_valid()) {
                lua_pushfstring(L, "malformed flowing mask name: %s", flowing_mask.value());
                return false;
            }
        }
    }

    lua_pop(L, 1);

    return true;
}

static bool add_fluid(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx, fluid_id_type& out_fluid_id)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "fluids.add: malformed name: %s", raw_name);
        return false;
    }

    id = make_unique_id(id, ctx);

    FluidDefinition def {};

    def.gravity = FLUID_GRAVITY_DOWN;
    def.opaque = false;

    def.fog_density = 1.0f;
    def.fog_color.setOnes();

    def.tint_name = {};
    def.tint = TINT_ID_NULL;

    if(!parse_definition(L, def_idx, def, ctx)) {
        return false;
    }

    out_fluid_id = ctx->register_fluid(id, std::move(def));

    return true;
}

static int api_get(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto raw_name = luaL_checkstring(L, 1);
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushinteger(L, FLUID_ID_NULL);
        return 1;
    }

    lua_pushinteger(L, fluid_registry::find(id));
    return 1;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto raw_name = luaL_checkstring(L, 1);

    int def_idx = 2;

    luaL_checktype(L, def_idx, LUA_TTABLE);

    fluid_id_type fluid_id = FLUID_ID_NULL;

    if(!add_fluid(L, ctx, raw_name, def_idx, fluid_id)) {
        return lua_error(L);
    }

    lua_pushinteger(L, fluid_id);
    return 1;
}

void api_shared::open_fluids(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    auto L = lua.get();

    lua_newtable(L);

    lua_pushinteger(L, FLUID_GRAVITY_DOWN);
    lua_setfield(L, -2, "GRAVITY_DOWN");
    lua_pushinteger(L, FLUID_GRAVITY_UP);
    lua_setfield(L, -2, "GRAVITY_UP");

    lua_pushinteger(L, FLUID_ID_NULL);
    lua_setfield(L, -2, "NULL_FLUID");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, api_add, 1);
    lua_setfield(L, -2, "add");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, api_get, 1);
    lua_setfield(L, -2, "get");

    lua_setglobal(L, "fluids");
}
