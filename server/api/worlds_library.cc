#include "server/pch.hh"

#include "server/api/worlds_library.hh"

#include "core/identifier.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/world_definition.hh"
#include "shared/world/world_registry.hh"

#include "server/world/generator_map.hh"

static bool fail(lua_State* L, int base, std::string_view message)
{
    lua_settop(L, base);
    lua_pushlstring(L, message.data(), message.size());
    return false;
}

static bool parse_name_list(lua_State* L, int base, int table_idx, const char* key, const ModContext* ctx, std::vector<Identifier>& out)
{
    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "name list must be a table");
    }

    auto count = static_cast<std::size_t>(lua_rawlen(L, -1));

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            lua_pop(L, 2);
            return fail(L, base, "name list entry must be a string");
        }

        out.push_back(Identifier::from_string(value.value(), ctx->name_space()));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_color(lua_State* L, int base, int table_idx, const char* key, Eigen::Vector3f& out)
{
    auto value = utils::require_fvec<3>(L, table_idx, key);

    if(!value.has_value()) {
        return fail(L, base, "visuals color must hold three numbers");
    }

    out = value.value().cast<float>();
    return true;
}

static bool parse_gradient(lua_State* L, int base, int table_idx, const char* key, WorldVisuals::Sky::Gradient& out)
{
    lua_getfield(L, table_idx, key);

    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "visuals sky gradient must be a table");
    }

    auto index = lua_gettop(L);

    if(!parse_color(L, base, index, "horizon", out.horizon)) {
        return false;
    }

    if(!parse_color(L, base, index, "zenith", out.zenith)) {
        return false;
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_sky(lua_State* L, int base, int table_idx, WorldVisuals::Sky& out)
{
    if(!parse_gradient(L, base, table_idx, "day", out.day)) {
        return false;
    }

    if(!parse_gradient(L, base, table_idx, "night", out.night)) {
        return false;
    }

    auto twilight_height = utils::require_number(L, table_idx, "twilight_height");

    if(!twilight_height.has_value()) {
        return fail(L, base, "visuals sky twilight_height must be a number");
    }

    out.twilight_height = static_cast<float>(twilight_height.value());
    return true;
}

static bool parse_sun(lua_State* L, int base, int table_idx, WorldVisuals::Sun& out)
{
    auto tilt = utils::require_number(L, table_idx, "tilt");

    if(!tilt.has_value()) {
        return fail(L, base, "visuals sun tilt must be a number");
    }

    out.tilt = static_cast<float>(tilt.value());
    return true;
}

static bool parse_moon(lua_State* L, int base, int table_idx, WorldVisuals::Moon& out)
{
    return parse_color(L, base, table_idx, "color", out.color);
}

static bool parse_stars(lua_State* L, int base, int table_idx, WorldVisuals::Stars& out)
{
    auto count = utils::require_integer(L, table_idx, "count");

    if(!count.has_value()) {
        return fail(L, base, "visuals stars count must be an integer");
    }

    out.count = static_cast<std::uint32_t>(count.value());
    return true;
}

static bool parse_visuals(lua_State* L, int base, int table_idx, WorldVisuals& out)
{
    lua_getfield(L, table_idx, "visuals");

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "visuals must be a table");
    }

    auto index = lua_gettop(L);

    lua_getfield(L, index, "sky");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fail(L, base, "visuals sky must be a table");
        }

        WorldVisuals::Sky sky;

        auto sky_idx = lua_gettop(L);

        if(!parse_sky(L, base, sky_idx, sky)) {
            return false;
        }

        out.sky = std::move(sky);

        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
    }

    lua_getfield(L, index, "sun");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fail(L, base, "visuals sun must be a table");
        }

        WorldVisuals::Sun sun;

        auto sun_idx = lua_gettop(L);

        if(!parse_sun(L, base, sun_idx, sun)) {
            return false;
        }

        out.sun = std::move(sun);

        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
    }

    lua_getfield(L, index, "moon");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fail(L, base, "visuals moon must be a table");
        }

        WorldVisuals::Moon moon;

        auto moon_idx = lua_gettop(L);

        if(!parse_moon(L, base, moon_idx, moon)) {
            return false;
        }

        out.moon = std::move(moon);

        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
    }

    lua_getfield(L, index, "stars");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fail(L, base, "visuals stars must be a table");
        }

        WorldVisuals::Stars stars;

        auto stars_idx = lua_gettop(L);

        if(!parse_stars(L, base, stars_idx, stars)) {
            return false;
        }

        out.stars = std::move(stars);

        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
    }

    lua_getfield(L, index, "clouds");

    if(!lua_isnoneornil(L, -1)) {
        if(!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fail(L, base, "visuals clouds must be a table");
        }

        out.clouds = WorldVisuals::Clouds {};

        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_string_list(lua_State* L, int base, int table_idx, const char* key, std::vector<std::string>& out)
{
    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, "biome_selector axes must be a table");
    }

    auto count = static_cast<std::size_t>(lua_rawlen(L, -1));

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return fail(L, base, "biome_selector axes entry must be a string");
        }

        out.emplace_back(value.value());

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_number_list(lua_State* L, int base, int table_idx, const char* key, std::vector<float>& out)
{
    lua_getfield(L, table_idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, "biome_selector weights must be a table");
    }

    auto count = static_cast<std::size_t>(lua_rawlen(L, -1));

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, -1, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_number(L, -1);

        if(!value.has_value()) {
            return fail(L, base, "biome_selector weights entry must be a number");
        }

        out.emplace_back(static_cast<float>(value.value()));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_target_list(lua_State* L, int base, int target_idx, std::vector<std::uint8_t>& out)
{
    auto count = static_cast<std::size_t>(lua_rawlen(L, target_idx));
    out.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, target_idx, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return fail(L, base, "biome_selector target entry must be an integer");
        }

        if(value.value() < 0 || value.value() > 99) {
            return fail(L, base, "biome_selector target value out of range [0..99]");
        }

        out.emplace_back(static_cast<std::uint8_t>(value.value()));

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_nearest_selector(lua_State* L, int base, int sel_idx, const ModContext* ctx, BiomeSelector& selector)
{
    if(!parse_string_list(L, base, sel_idx, "axes", selector.axes)) {
        return false;
    }

    if(!parse_number_list(L, base, sel_idx, "weights", selector.weights)) {
        return false;
    }

    lua_getfield(L, sel_idx, "entries");

    if(!lua_istable(L, -1)) {
        return fail(L, base, "biome_selector entries must be a table");
    }

    auto entries_idx = lua_gettop(L);
    auto count = static_cast<std::size_t>(lua_rawlen(L, entries_idx));

    selector.entries.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(L, entries_idx, static_cast<lua_Integer>(i + 1));

        auto entry_idx = lua_gettop(L);

        if(!lua_istable(L, entry_idx)) {
            return fail(L, base, "biome_selector entry must be a table");
        }

        auto biome = utils::require_string(L, entry_idx, "biome");

        if(!biome.has_value()) {
            return fail(L, base, "biome_selector entry biome must be a string");
        }

        BiomeSelector::Entry entry {};
        entry.biome_name = Identifier::from_string(biome.value(), ctx->name_space());

        lua_getfield(L, entry_idx, "target");

        if(!lua_istable(L, -1)) {
            return fail(L, base, "biome_selector entry target must be a table");
        }

        auto target_idx = lua_gettop(L);

        if(!parse_target_list(L, base, target_idx, entry.target)) {
            return false;
        }

        lua_pop(L, 1);

        auto priority = utils::opt_integer(L, entry_idx, "priority", 0);

        if(!priority.has_value() || priority.value() < 0) {
            return fail(L, base, "biome_selector entry priority must be non-negative");
        }

        entry.priority = static_cast<unsigned>(priority.value());

        auto offset = utils::opt_number(L, entry_idx, "offset", 0.0);

        if(!offset.has_value()) {
            return fail(L, base, "biome_selector entry offset must be a number");
        }

        entry.offset = static_cast<float>(offset.value());

        selector.entries.push_back(std::move(entry));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_grid_selector(lua_State* L, int base, int sel_idx, const ModContext* ctx, BiomeSelector& selector)
{
    if(!parse_string_list(L, base, sel_idx, "axes", selector.axes)) {
        return false;
    }

    if(selector.axes.size() != 2) {
        return fail(L, base, "biome_selector grid requires exactly two axes");
    }

    lua_getfield(L, sel_idx, "weights");

    if(!lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "biome_selector grid does not use weights");
    }

    lua_pop(L, 1);

    lua_getfield(L, sel_idx, "entries");

    if(!lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return fail(L, base, "biome_selector grid does not use entries");
    }

    lua_pop(L, 1);

    auto grid_size = utils::opt_integer(L, sel_idx, "grid_size", 0);

    if(!grid_size.has_value() || grid_size.value() < 1 || grid_size.value() > static_cast<lua_Integer>(BIOME_SELECTOR_MAX_GRID_SIZE)) {
        return fail(L, base, "biome_selector grid_size is out of range");
    }

    selector.grid_size = static_cast<std::size_t>(grid_size.value());

    lua_getfield(L, sel_idx, "lookup");

    if(!lua_istable(L, -1)) {
        return fail(L, base, "biome_selector grid lookup must be a table");
    }

    auto lookup_idx = lua_gettop(L);
    auto rows = static_cast<std::size_t>(lua_rawlen(L, lookup_idx));

    if(rows != selector.grid_size) {
        return fail(L, base, "biome_selector grid lookup row count does not match grid_size");
    }

    selector.lookup_names.reserve(selector.grid_size * selector.grid_size);

    for(std::size_t row = 0; row < selector.grid_size; row += 1) {
        lua_rawgeti(L, lookup_idx, static_cast<lua_Integer>(row + 1));

        if(!lua_istable(L, -1)) {
            return fail(L, base, "biome_selector grid lookup row must be a table");
        }

        auto row_idx = lua_gettop(L);
        auto columns = static_cast<std::size_t>(lua_rawlen(L, row_idx));

        if(columns != selector.grid_size) {
            return fail(L, base, "biome_selector grid lookup column count does not match grid_size");
        }

        for(std::size_t column = 0; column < selector.grid_size; column += 1) {
            lua_rawgeti(L, row_idx, static_cast<lua_Integer>(column + 1));

            auto name = utils::require_string(L, -1);

            if(!name.has_value()) {
                return fail(L, base, "biome_selector grid lookup entry must be a string");
            }

            auto id = Identifier::from_string(name.value(), ctx->name_space());

            if(!id.is_valid()) {
                return fail(L, base, "biome_selector grid lookup entry is not a valid identifier");
            }

            selector.lookup_names.push_back(id);

            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool parse_biome_selector(lua_State* L, int base, int table_idx, const ModContext* ctx, BiomeSelector& selector)
{
    lua_getfield(L, table_idx, "biome_selector");

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }

    if(!lua_istable(L, -1)) {
        return fail(L, base, "biome_selector must be a table");
    }

    auto sel_idx = lua_gettop(L);

    auto strategy = utils::opt_integer(L, sel_idx, "strategy", static_cast<lua_Integer>(BIOME_SELECTION_CONSTANT));

    if(!strategy.has_value()) {
        return fail(L, base, "biome_selector strategy must be an integer");
    }

    if(strategy.value() == BIOME_SELECTION_NEAREST) {
        selector.strategy = BIOME_SELECTION_NEAREST;

        if(!parse_nearest_selector(L, base, sel_idx, ctx, selector)) {
            return false;
        }
    }
    else if(strategy.value() == BIOME_SELECTION_CONSTANT) {
        selector.strategy = BIOME_SELECTION_CONSTANT;

        auto biome = utils::require_string(L, sel_idx, "biome");

        if(!biome.has_value()) {
            return fail(L, base, "biome_selector constant biome must be a string");
        }

        selector.constant_biome_name = Identifier::from_string(biome.value(), ctx->name_space());
    }
    else if(strategy.value() == BIOME_SELECTION_GRID) {
        selector.strategy = BIOME_SELECTION_GRID;

        if(!parse_grid_selector(L, base, sel_idx, ctx, selector)) {
            return false;
        }
    }
    else {
        return fail(L, base, "biome_selector unknown strategy");
    }

    lua_pop(L, 1);
    return true;
}

static void push_world_error(lua_State* L, std::string_view prefix, std::string_view name, std::string_view suffix)
{
    lua_pushlstring(L, prefix.data(), prefix.size());
    lua_pushlstring(L, name.data(), name.size());
    lua_pushlstring(L, suffix.data(), suffix.size());
    lua_concat(L, 3);
}

static bool do_add(lua_State* L, ModContext* ctx)
{
    auto base = lua_gettop(L);

    auto raw_name = utils::require_string(L, 1);

    if(!raw_name.has_value()) {
        return false;
    }

    if(!lua_istable(L, 2)) {
        lua_pushliteral(L, "worlds.add: config must be a table");
        return false;
    }

    auto id = Identifier::from_string(raw_name.value(), ctx->name_space());

    if(!id.is_valid()) {
        push_world_error(L, "worlds.add: malformed name: ", raw_name.value(), "");
        return false;
    }

    auto definition = world_registry::add(id);

    if(definition == nullptr) {
        push_world_error(L, "worlds.add: ", raw_name.value(), " is already registered");
        return false;
    }

    auto generator = utils::opt_string(L, 2, "generator", {});

    if(!generator.has_value()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": invalid generator");
        return false;
    }

    if(generator->empty()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": generator is required");
        return false;
    }

    definition->generator = std::string(generator.value());

    auto directory = utils::opt_string(L, 2, "directory", {});

    if(!directory.has_value()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": invalid directory");
        return false;
    }

    definition->directory = std::string(directory.value());

    auto min_y = utils::opt_integer(L, 2, "min_y", -64);

    if(!min_y.has_value()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": invalid min_y");
        return false;
    }

    definition->min_y = static_cast<BlockPos::value_type>(min_y.value());

    auto max_y = utils::opt_integer(L, 2, "max_y", 255);

    if(!max_y.has_value()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": invalid max_y");
        return false;
    }

    definition->max_y = static_cast<BlockPos::value_type>(max_y.value());

    auto spawn = utils::opt_ivec<3>(L, 2, "spawn", Eigen::Vector<lua_Integer, 3>(0, 32, 0));

    if(!spawn.has_value()) {
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": invalid spawn");
        return false;
    }

    definition->spawn = BlockPos(spawn->x(), spawn->y(), spawn->z());

    if(!parse_biome_selector(L, base, 2, ctx, definition->biome_selector)) {
        world_registry::remove(id);
        return false;
    }

    if(!parse_name_list(L, base, 2, "weather", ctx, definition->weather_names)) {
        world_registry::remove(id);
        return false;
    }

    if(!parse_visuals(L, base, 2, definition->visuals)) {
        world_registry::remove(id);
        return false;
    }

    auto generator_def = generator_map::find(definition->generator);

    if(generator_def == nullptr) {
        world_registry::remove(id);
        lua_pushliteral(L, "worlds.add: ");
        lua_pushlstring(L, raw_name->data(), raw_name->size());
        lua_pushliteral(L, ": unknown generator \"");
        lua_pushlstring(L, definition->generator.data(), definition->generator.size());
        lua_pushliteral(L, "\"");
        lua_concat(L, 5);

        return false;
    }

    lua_getfield(L, 2, "generator_settings");

    auto settings_idx = 0;

    if(lua_istable(L, -1)) {
        settings_idx = lua_gettop(L);
    }
    else if(!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        world_registry::remove(id);
        push_world_error(L, "worlds.add: ", raw_name.value(), ": generator_settings must be a table");
        return false;
    }

    auto state = generator_def->create_state(L, settings_idx, *definition);

    if(state == nullptr) {
        world_registry::remove(id);
        return false;
    }

    definition->generator_state = std::move(state);
    lua_pop(L, 1);

    return true;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    if(!do_add(L, ctx)) {
        return lua_error(L);
    }

    return 0;
}

void api_server::open_worlds(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // Worlds

    lua_pushinteger(L, BIOME_SELECTION_NEAREST);
    lua_setfield(L, -2, "SELECTION_NEAREST");
    lua_pushinteger(L, BIOME_SELECTION_GRID);
    lua_setfield(L, -2, "SELECTION_GRID");
    lua_pushinteger(L, BIOME_SELECTION_CONSTANT);
    lua_setfield(L, -2, "SELECTION_CONSTANT");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_setglobal(L, "worlds");
}
