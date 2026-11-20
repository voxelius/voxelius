#include "shared/pch.hh"

#include "shared/api/biomes_library.hh"

#include "core/identifier.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/biome_registry.hh"

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_biome(candidate) == BIOME_ID_NULL && biome_registry::find(candidate) == BIOME_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto candidate = Identifier::from_parts(id.name_space(), std::format("{}~{}", id.value(), n));

        if(is_free(candidate)) {
            msg::log_warning("biomes.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("biomes.add: {}: out of resolvable names", id.full_string());
    return id;
}

static bool parse_scatter_entry(lua_State* L, int entry_idx, BiomeScatterEntry& entry, ModContext* ctx)
{
    auto feature = utils::require_string(L, entry_idx, "feature");

    if(!feature.has_value()) {
        return false;
    }

    entry.feature = Identifier::from_string(feature.value(), ctx->name_space());

    auto chance = utils::opt_number(L, entry_idx, "chance", 0.5);

    if(!chance.has_value()) {
        return false;
    }

    entry.chance = static_cast<float>(chance.value());

    auto need_above = utils::opt_bitmask<unsigned>(L, entry_idx, "need_above", 0);

    if(!need_above.has_value()) {
        return false;
    }

    entry.need_above = static_cast<block_tag_bit>(need_above.value());

    auto need_below = utils::opt_bitmask<unsigned>(L, entry_idx, "need_below", 0);

    if(!need_below.has_value()) {
        return false;
    }

    entry.need_below = static_cast<block_tag_bit>(need_below.value());

    auto group = utils::opt_string(L, entry_idx, "group", {});

    if(!group.has_value()) {
        return false;
    }

    if(group->empty()) {
        entry.group_hash = 0;
    }
    else {
        entry.group_hash = std::hash<std::string_view>()(*group);
    }

    auto padding = utils::opt_integer(L, entry_idx, "padding", 0);

    if(!padding.has_value()) {
        return false;
    }

    if(padding.value() < 0) {
        lua_pushliteral(L, "padding must be non-negative");
        return false;
    }

    entry.padding = static_cast<unsigned>(padding.value());

    auto edge = utils::opt_integer(L, entry_idx, "edge", 0);

    if(!edge.has_value()) {
        return false;
    }

    if(edge.value() < 0) {
        lua_pushliteral(L, "edge must be non-negative");
        return false;
    }

    entry.edge = static_cast<unsigned>(edge.value());

    return true;
}

static bool parse_palette_entry(lua_State* L, int entry_idx, BiomePaletteEntry& entry, ModContext* ctx)
{
    auto name = utils::require_string(L, entry_idx, "name");

    if(!name.has_value()) {
        return false;
    }

    entry.name = Identifier::from_string(name.value(), ctx->name_space());

    lua_getfield(L, entry_idx, "states");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, -1)));
        return false;
    }

    entry.states.clear();

    lua_pushnil(L);

    while(lua_next(L, -2)) {
        auto key_str = utils::require_string(L, -2);

        if(!key_str.has_value()) {
            return false;
        }

        auto value_str = utils::require_string(L, -1);

        if(!value_str.has_value()) {
            return false;
        }

        entry.states.try_emplace(std::string(key_str.value()), std::string(value_str.value()));

        lua_pop(L, 1);
    }

    entry.cached = BIOME_ID_NULL;

    lua_pop(L, 1);

    return true;
}

static bool parse_palette(lua_State* L, int palette_idx, BiomeDefinition& def, ModContext* ctx)
{
    const std::array palette_fields = {
        std::make_pair("basic", &def.palette_basic),
        std::make_pair("filler", &def.palette_filler),
        std::make_pair("surface", &def.palette_surface),
        std::make_pair("fluid", &def.palette_fluid),
    };

    for(const auto& [field_name, field_ptr] : palette_fields) {
        lua_getfield(L, palette_idx, field_name);

        if(!lua_isnil(L, -1)) {
            if(!parse_palette_entry(L, lua_gettop(L), field_ptr[0], ctx)) {
                return false;
            }
        }

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_definition(lua_State* L, int def_idx, BiomeDefinition& def, ModContext* ctx)
{
    const std::array climate_fields = {
        std::make_pair("temperature", &def.temperature),
        std::make_pair("humidity", &def.humidity),
        std::make_pair("continentalness", &def.continentalness),
        std::make_pair("erosion", &def.erosion),
        std::make_pair("weirdness", &def.weirdness),
    };

    for(const auto& [field_name, field_ptr] : climate_fields) {
        lua_getfield(L, def_idx, field_name);

        if(!lua_isnil(L, -1)) {
            auto value = utils::require_integer(L, -1);

            if(!value.has_value()) {
                return false;
            }

            if(value.value() < 0 || value.value() > 99) {
                lua_pushliteral(L, "climate value out of range [0..99]");
                return false;
            }

            field_ptr[0] = static_cast<std::uint8_t>(value.value());
        }

        lua_pop(L, 1);
    }

    auto priority = utils::opt_integer(L, def_idx, "priority", 0);

    if(!priority.has_value()) {
        return false;
    }

    if(priority.value() < 0) {
        lua_pushliteral(L, "priority must be non-negative");
        return false;
    }

    def.priority = static_cast<unsigned>(priority.value());

    auto offset = utils::opt_number(L, def_idx, "offset", 0.0);

    if(!offset.has_value()) {
        return false;
    }

    def.offset = static_cast<float>(offset.value());

    lua_getfield(L, def_idx, "palette");

    if(!lua_isnil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, -1)));
            return false;
        }

        if(!parse_palette(L, lua_gettop(L), def, ctx)) {
            return false;
        }
    }

    lua_pop(L, 1);

    lua_getfield(L, def_idx, "scatter");

    def.scatter.clear();

    if(!lua_isnil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, -1)));
            return false;
        }

        def.scatter.reserve(lua_rawlen(L, -1));

        lua_pushnil(L);

        while(lua_next(L, -2)) {
            auto entry_idx = lua_gettop(L);

            if(!lua_istable(L, entry_idx)) {
                lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, entry_idx)));
                return false;
            }

            BiomeScatterEntry entry {};

            if(!parse_scatter_entry(L, entry_idx, entry, ctx)) {
                return false;
            }

            def.scatter.push_back(std::move(entry));

            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1);

    lua_getfield(L, def_idx, "tints");

    if(!lua_isnil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, -1)));
            return false;
        }

        def.tint_map.clear();

        lua_pushnil(L);

        while(lua_next(L, -2)) {
            auto key_str = utils::require_string(L, -2);

            if(!key_str.has_value()) {
                return false;
            }

            auto value_vec = utils::require_fvec<3>(L, -1);

            if(!value_vec.has_value()) {
                return false;
            }

            auto key_id = Identifier::from_string(key_str.value(), ctx->name_space());
            auto color = value_vec->cast<float>().cwiseMin(1.0f).cwiseMax(0.0f);

            def.tint_map.insert_or_assign(std::move(key_id), std::move(color));

            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1);

    return true;
}

static bool add_biome(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx, biome_id_type& out_biome_id)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "biomes.add: malformed name: %s", raw_name);
        return false;
    }

    id = make_unique_id(id, ctx);

    BiomeDefinition def {};

    def.temperature = 50;
    def.humidity = 50;
    def.continentalness = 50;
    def.erosion = 50;
    def.weirdness = 50;
    def.offset = 0.0f;

    def.priority = 0;

    if(!parse_definition(L, def_idx, def, ctx)) {
        return false;
    }

    out_biome_id = ctx->register_biome(std::move(id), std::move(def));

    return true;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto raw_name = luaL_checkstring(L, 1);

    int def_idx = 2;

    luaL_checktype(L, def_idx, LUA_TTABLE);

    biome_id_type biome_id = BIOME_ID_NULL;

    if(!add_biome(L, ctx, raw_name, def_idx, biome_id)) {
        return lua_error(L);
    }

    lua_pushinteger(L, biome_id);
    return 1;
}

void api::open_biomes_library(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // biomes

    lua_pushinteger(L, BIOME_REALM_SURFACE);
    lua_setfield(L, -2, "REALM_SURFACE");
    lua_pushinteger(L, BIOME_REALM_UNDERGROUND);
    lua_setfield(L, -2, "REALM_UNDERGROUND");
    lua_pushinteger(L, BIOME_REALM_THE_DEPTHS);
    lua_setfield(L, -2, "REALM_THE_DEPTHS");
    lua_pushinteger(L, BIOME_REALM_SKY);
    lua_setfield(L, -2, "REALM_SKY");

    lua_pushinteger(L, BIOME_ID_NULL);
    lua_setfield(L, -2, "NULL_BIOME");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_setglobal(L, "biomes");
}
