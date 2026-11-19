#include "shared/pch.hh"

#include "shared/api/blocks_library.hh"

#include "core/identifier.hh"
#include "core/utils/crc64.hh"

#include "shared/mod_context.hh"
#include "shared/utils/lua.hh"
#include "shared/world/block_registry.hh"

class BlockDefReader final {
public:
    explicit BlockDefReader(lua_State* L, int def_idx, int proto_idx);
    bool try_push(const char* key) const;

public:
    lua_State* L;
    int def_idx;
    int proto_idx;
};

BlockDefReader::BlockDefReader(lua_State* L, int def_idx, int proto_idx) : L(L), def_idx(def_idx), proto_idx(proto_idx)
{
    assert(L);
    assert(def_idx > 0);
    assert(proto_idx >= 0);
}

bool BlockDefReader::try_push(const char* key) const
{
    lua_getfield(L, def_idx, key);

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        if(proto_idx) {
            lua_getfield(L, proto_idx, key);

            if(lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return false;
            }

            return true;
        }

        return false;
    }

    return true;
}

static block_light_type clamp_light(lua_Integer value)
{
    return static_cast<block_light_type>(std::clamp<lua_Integer>(value, BLOCK_LIGHT_MIN, BLOCK_LIGHT_MAX));
}

static bool parse_drop_effects(lua_State* L, int when_idx, BlockDrop& drop, ModContext* ctx)
{
    lua_getfield(L, when_idx, "effects");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    auto effects_idx = lua_gettop(L);
    auto effects_count = lua_rawlen(L, effects_idx);

    drop.cond_effects.clear();
    drop.cond_effects.reserve(effects_count);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(effects_count); ++i) {
        lua_rawgeti(L, effects_idx, i);

        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        drop.cond_effects.emplace_back(Identifier::from_string(value.value(), ctx->name_space()));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_drop_tools(lua_State* L, int when_idx, BlockDrop& drop)
{
    auto mask = utils::opt_bitmask<unsigned>(L, when_idx, "tools", static_cast<unsigned>(drop.cond_tool_bits));

    if(!mask.has_value()) {
        return false;
    }

    drop.cond_tool_bits = static_cast<block_tool_bit>(mask.value());

    return true;
}

static bool parse_drop_when(lua_State* L, int entry_idx, BlockDrop& drop, ModContext* ctx)
{
    lua_getfield(L, entry_idx, "when");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    auto when_idx = lua_gettop(L);

    if(!parse_drop_effects(L, when_idx, drop, ctx)) {
        return false;
    }

    if(!parse_drop_tools(L, when_idx, drop)) {
        return false;
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_drop_items(lua_State* L, int entry_idx, BlockDrop& drop, ModContext* ctx)
{
    lua_getfield(L, entry_idx, "items");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    auto items_idx = lua_gettop(L);
    auto items_count = lua_rawlen(L, items_idx);

    drop.items.clear();
    drop.items.reserve(items_count);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(items_count); ++i) {
        lua_rawgeti(L, items_idx, i);

        auto item_idx = lua_gettop(L);

        BlockDropItem item {};

        auto name = utils::require_string(L, item_idx, "name");

        if(!name.has_value()) {
            return false;
        }

        item.name = Identifier::from_string(name.value(), ctx->name_space());

        auto count = utils::opt_integer(L, item_idx, "count", 1);

        if(!count.has_value()) {
            return false;
        }

        item.count = static_cast<unsigned>(count.value());

        drop.items.push_back(std::move(item));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_drops(lua_State* L, int drops_idx, ModContext* ctx, std::vector<BlockDrop>& out)
{
    auto count = lua_rawlen(L, drops_idx);

    out.clear();
    out.reserve(count);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
        lua_rawgeti(L, drops_idx, i);

        auto entry_idx = lua_gettop(L);

        BlockDrop drop {};
        drop.cond_tool_bits = BLOCK_TOOL_NONE;

        if(!parse_drop_when(L, entry_idx, drop, ctx)) {
            return false;
        }

        if(!parse_drop_items(L, entry_idx, drop, ctx)) {
            return false;
        }

        out.push_back(std::move(drop));

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_overrides(lua_State* L, int idx, ModContext* ctx, BlockOverridePatch& patch)
{
    BlockDefReader reader(L, idx, 0);

    if(reader.try_push("render")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.render = static_cast<block_render>(value.value());

        lua_pop(L, 1);
    }

    if(reader.try_push("albedo")) {
        vx::hash_map<std::string, std::vector<Identifier>> albedo;
        auto albedo_idx = lua_gettop(L);

        lua_pushnil(L);

        while(lua_next(L, albedo_idx)) {
            auto slot_name = utils::require_string(L, -2);

            if(!slot_name.has_value()) {
                return false;
            }

            auto slot = std::string(slot_name.value());
            auto count = lua_rawlen(L, -1);

            std::vector<Identifier> variants;
            variants.reserve(count);

            for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
                lua_rawgeti(L, -1, i);

                auto variant_name = utils::require_string(L, -1);

                if(!variant_name.has_value()) {
                    return false;
                }

                variants.emplace_back(Identifier::from_string(variant_name.value(), ctx->name_space()));
                lua_pop(L, 1);
            }

            albedo.insert_or_assign(std::move(slot), std::move(variants));
            lua_pop(L, 1);
        }

        patch.albedo = std::move(albedo);
        lua_pop(L, 1);
    }

    if(reader.try_push("masks")) {
        vx::hash_map<std::string, Identifier> masks;

        auto masks_idx = lua_gettop(L);

        lua_pushnil(L);

        while(lua_next(L, masks_idx)) {
            auto slot_name = utils::require_string(L, -2);

            if(!slot_name.has_value()) {
                return false;
            }

            auto slot = std::string(slot_name.value());

            auto mask_name = utils::require_string(L, -1);

            if(!mask_name.has_value()) {
                return false;
            }

            auto mask_id = Identifier::from_string(mask_name.value(), ctx->name_space());

            if(!mask_id.is_valid()) {
                lua_pushfstring(L, "malformed mask name: %s", mask_name.value());
                return false;
            }

            masks.insert_or_assign(std::move(slot), std::move(mask_id));
            lua_pop(L, 1);
        }

        patch.masks = std::move(masks);
        lua_pop(L, 1);
    }

    if(reader.try_push("animated")) {
        patch.animated = static_cast<bool>(lua_toboolean(L, -1));
        lua_pop(L, 1);
    }

    if(reader.try_push("model_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.model_name = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("model_offset")) {
        auto offset = utils::require_fvec<3>(L, lua_gettop(L));

        if(!offset.has_value()) {
            return false;
        }

        patch.model_offset = 0.0625f * offset->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("model_facing")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.model_facing = static_cast<block_face>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.bcoll_name = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_offset")) {
        auto offset = utils::require_fvec<3>(L, lua_gettop(L));

        if(!offset.has_value()) {
            return false;
        }

        patch.bcoll_offset = 0.0625f * offset->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_facing")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.bcoll_facing = static_cast<block_face>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        auto fluid_id = Identifier::from_string(value.value(), ctx->name_space());

        if(!fluid_id.is_valid()) {
            lua_pushfstring(L, "malformed fluid name: %s", value.value());
            return false;
        }

        patch.fluid_name = std::move(fluid_id);
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_level")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.fluid_level = static_cast<unsigned>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_faces")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.fluid_faces = static_cast<std::uint32_t>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("health")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.health = static_cast<unsigned>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("sound")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.sound_set = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("emission")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.emission = clamp_light(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("dissipation")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.dissipation = clamp_light(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("touch")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        patch.touch = static_cast<block_touch>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("touch_coeffs")) {
        auto coeffs = utils::require_fvec<3>(L, lua_gettop(L));

        if(!coeffs.has_value()) {
            return false;
        }

        patch.touch_coeffs = coeffs->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("tags")) {
        auto mask = utils::require_bitmask<unsigned>(L, lua_gettop(L));

        if(!mask.has_value()) {
            return false;
        }

        patch.tags = static_cast<block_tag_bit>(mask.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("replaceable")) {
        patch.replaceable = static_cast<bool>(lua_toboolean(L, -1));
        lua_pop(L, 1);
    }

    if(reader.try_push("drops")) {
        std::vector<BlockDrop> drops;

        if(!parse_drops(L, lua_gettop(L), ctx, drops)) {
            return false;
        }

        patch.drops = std::move(drops);
        lua_pop(L, 1);
    }

    return true;
}

static bool parse_definition(const BlockDefReader& reader, ModContext* ctx, BlockDefinition& def)
{
    auto L = reader.L;

    if(reader.try_push("render")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.render = static_cast<block_render>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("model_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.model_name = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("model_offset")) {
        auto offset = utils::require_fvec<3>(L, lua_gettop(L));

        if(!offset.has_value()) {
            return false;
        }

        def.model_offset = 0.0625f * offset->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("model_facing")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.model_facing = static_cast<block_face>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.bcoll_name = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_offset")) {
        auto offset = utils::require_fvec<3>(L, lua_gettop(L));

        if(!offset.has_value()) {
            return false;
        }

        def.bcoll_offset = 0.0625f * offset->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("bcoll_facing")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.bcoll_facing = static_cast<block_face>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_name")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        auto fluid_id = Identifier::from_string(value.value(), ctx->name_space());

        if(!fluid_id.is_valid()) {
            lua_pushfstring(L, "malformed fluid name: %s", value.value());
            return false;
        }

        def.fluid_name = std::move(fluid_id);
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_level")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.fluid_level = static_cast<unsigned>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("fluid_faces")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.fluid_faces = static_cast<std::uint32_t>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("animated")) {
        def.animated = static_cast<bool>(lua_toboolean(L, -1));
        lua_pop(L, 1);
    }

    if(reader.try_push("albedo")) {
        auto albedo_idx = lua_gettop(L);

        def.albedo.clear();

        lua_pushnil(L);

        while(lua_next(L, albedo_idx)) {
            auto slot_name = utils::require_string(L, -2);

            if(!slot_name.has_value()) {
                return false;
            }

            auto slot = std::string(slot_name.value());
            auto count = lua_rawlen(L, -1);

            std::vector<Identifier> variants;
            variants.reserve(count);

            for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
                lua_rawgeti(L, -1, i);

                auto variant_name = utils::require_string(L, -1);

                if(!variant_name.has_value()) {
                    return false;
                }

                variants.emplace_back(Identifier::from_string(variant_name.value(), ctx->name_space()));
                lua_pop(L, 1);
            }

            def.albedo.insert_or_assign(std::move(slot), std::move(variants));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    if(reader.try_push("masks")) {
        auto masks_idx = lua_gettop(L);

        def.masks.clear();

        lua_pushnil(L);

        while(lua_next(L, masks_idx)) {
            auto slot_name = utils::require_string(L, -2);

            if(!slot_name.has_value()) {
                return false;
            }

            auto slot = std::string(slot_name.value());

            auto mask_name = utils::require_string(L, -1);

            if(!mask_name.has_value()) {
                return false;
            }

            auto mask_id = Identifier::from_string(mask_name.value(), ctx->name_space());

            if(!mask_id.is_valid()) {
                lua_pushfstring(L, "malformed mask name: %s", mask_name.value());
                return false;
            }

            def.masks.insert_or_assign(std::move(slot), std::move(mask_id));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    if(reader.try_push("health")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.health = static_cast<unsigned>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("sound")) {
        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.sound_set = Identifier::from_string(value.value(), ctx->name_space());
        lua_pop(L, 1);
    }

    if(reader.try_push("emission")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.emission = clamp_light(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("dissipation")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.dissipation = clamp_light(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("touch")) {
        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return false;
        }

        def.touch = static_cast<block_touch>(value.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("touch_coeffs")) {
        auto coeffs = utils::require_fvec<3>(L, lua_gettop(L));

        if(!coeffs.has_value()) {
            return false;
        }

        def.touch_coeffs = coeffs->cast<float>();
        lua_pop(L, 1);
    }

    if(reader.try_push("tags")) {
        auto mask = utils::require_bitmask<unsigned>(L, lua_gettop(L));

        if(!mask.has_value()) {
            return false;
        }

        def.tags = static_cast<block_tag_bit>(mask.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("replaceable")) {
        def.replaceable = static_cast<bool>(lua_toboolean(L, -1));
        lua_pop(L, 1);
    }

    if(reader.try_push("tools")) {
        auto mask = utils::require_bitmask<unsigned>(L, lua_gettop(L));

        if(!mask.has_value()) {
            return false;
        }

        def.tools = static_cast<block_tool_bit>(mask.value());
        lua_pop(L, 1);
    }

    if(reader.try_push("drops")) {
        if(!parse_drops(L, lua_gettop(L), ctx, def.drops)) {
            return false;
        }

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_state_hint(lua_State* L, int entry_idx, BlockStateDecl& decl, BlockFamily& family)
{
    lua_getfield(L, entry_idx, "hint");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    auto hint_idx = lua_gettop(L);
    auto count = lua_rawlen(L, hint_idx);

    decl.hint.clear();
    decl.hint.reserve(count);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
        lua_rawgeti(L, hint_idx, i);

        auto value = utils::require_string(L, -1);

        if(!value.has_value()) {
            return false;
        }

        decl.hint.push_back(family.state_hash(value.value()));

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_states(lua_State* L, int idx, BlockFamily& family)
{
    lua_pushnil(L);

    while(lua_next(L, idx)) {
        auto state_name_str = utils::require_string(L, -2);

        if(!state_name_str.has_value()) {
            return false;
        }

        auto state_name = std::string(state_name_str.value());
        auto entry_idx = lua_gettop(L);

        BlockStateDecl decl {};

        auto default_value = utils::require_string(L, entry_idx, "default");

        if(!default_value.has_value()) {
            return false;
        }

        decl.default_value = family.state_hash(default_value.value());

        if(!parse_state_hint(L, entry_idx, decl, family)) {
            return false;
        }

        auto key = static_cast<blockstate_key_type>(utils::crc64(state_name.data(), state_name.size()));
        family.states.insert_or_assign(key, std::move(decl));

        lua_pop(L, 1);
    }

    return true;
}

static bool parse_variant_when(lua_State* L, int entry_idx, BlockVariantRule& rule, BlockFamily& family)
{
    lua_getfield(L, entry_idx, "when");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    auto when_idx = lua_gettop(L);

    lua_pushnil(L);

    while(lua_next(L, when_idx)) {
        auto state_name_str = utils::require_string(L, -2);

        if(!state_name_str.has_value()) {
            return false;
        }

        auto state_name = std::string(state_name_str.value());

        auto value_str = utils::require_string(L, -1);

        if(!value_str.has_value()) {
            return false;
        }

        auto key = static_cast<blockstate_key_type>(utils::crc64(state_name.data(), state_name.size()));
        auto value = family.state_hash(value_str.value());

        rule.when.try_emplace(key, value);

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_variant_overrides(lua_State* L, int entry_idx, BlockVariantRule& rule, ModContext* ctx)
{
    lua_getfield(L, entry_idx, "overrides");

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);

        return true;
    }

    if(!parse_overrides(L, lua_gettop(L), ctx, rule.overrides)) {
        return false;
    }

    lua_pop(L, 1);

    return true;
}

static bool parse_variants(lua_State* L, int idx, ModContext* ctx, BlockFamily& family)
{
    auto count = lua_rawlen(L, idx);

    family.variants.clear();
    family.variants.reserve(count);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
        lua_rawgeti(L, idx, i);

        auto entry_idx = lua_gettop(L);

        BlockVariantRule rule {};

        if(!parse_variant_when(L, entry_idx, rule, family)) {
            return false;
        }

        if(!parse_variant_overrides(L, entry_idx, rule, ctx)) {
            return false;
        }

        family.variants.push_back(std::move(rule));

        lua_pop(L, 1);
    }

    return true;
}

static BlockCallback parse_callback(lua_State* L, int table_idx, const char* key, ModContext* ctx)
{
    lua_getfield(L, table_idx, key);

    if(lua_isfunction(L, -1)) {
        BlockCallback callback {};
        callback.state = ctx->lua_state();
        callback.callback_ref = luaL_ref(L, LUA_REGISTRYINDEX); // pops the function
        return callback;
    }

    lua_pop(L, 1);
    return {};
}

static Identifier make_unique_id(const Identifier& id, const ModContext* ctx)
{
    auto is_free = [&](const Identifier& candidate) {
        return ctx->find_block(candidate) == BLOCK_ID_NULL && block_registry::find(candidate) == BLOCK_ID_NULL;
    };

    if(is_free(id)) {
        return id;
    }

    for(unsigned n = 1; n < 10000; ++n) {
        auto candidate = Identifier::from_parts(id.name_space(), std::format("{}~{}", id.value(), n));

        if(is_free(candidate)) {
            msg::log_warning("blocks.add: {} is already registered, using {} instead", id.full_string(), candidate.full_string());
            return candidate;
        }
    }

    msg::log_critical("blocks.add: {}: out of resolvable names", id.full_string());
    return id;
}

static int api_get(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto raw_name = luaL_checkstring(L, 1);
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushinteger(L, BLOCK_ID_NULL);
        return 1;
    }

    auto local_id = ctx->find_block(id);

    if(local_id == BLOCK_ID_NULL) {
        local_id = block_registry::find(id);
    }

    lua_pushinteger(L, local_id);
    return 1;
}

static int api_has_tag(lua_State* L)
{
    auto id = static_cast<block_id_type>(luaL_checkinteger(L, 1));
    auto tag = static_cast<block_tag_bit>(luaL_checkinteger(L, 2));

    lua_pushboolean(L, block_registry::has_tag_any(id, tag));
    return 1;
}

static int api_is_replaceable(lua_State* L)
{
    auto id = static_cast<block_id_type>(luaL_checkinteger(L, 1));

    if(id == BLOCK_ID_NULL) {
        lua_pushboolean(L, true);
        return 1;
    }

    auto def = block_registry::find_definition(id);
    lua_pushboolean(L, def && def->replaceable);
    return 1;
}

static bool add_block(lua_State* L, ModContext* ctx, const char* raw_name, int def_idx, int proto_idx, block_id_type& out_block_id)
{
    auto id = Identifier::from_string(raw_name, ctx->name_space());

    if(!id.is_valid()) {
        lua_pushfstring(L, "blocks.add: malformed name: %s", raw_name);
        return false;
    }

    id = make_unique_id(id, ctx);

    BlockDefinition def {};

    def.render = BLOCK_RENDER_NONE;
    def.animated = false;

    def.model_offset.setZero();
    def.model_facing = BLOCK_FACE_NORTH;

    def.bcoll_offset.setZero();
    def.bcoll_facing = BLOCK_FACE_NORTH;

    def.fluid = FLUID_ID_NULL;
    def.fluid_level = 0;
    def.fluid_faces = 0;
    def.fluid_name = std::nullopt;

    def.health = 0;
    def.tools = BLOCK_TOOL_NONE;

    def.emission = BLOCK_LIGHT_MIN;
    def.dissipation = BLOCK_LIGHT_MIN;

    def.touch = BLOCK_TOUCH_SOLID;
    def.touch_coeffs.setOnes();

    def.tags = static_cast<block_tag_bit>(0);
    def.replaceable = false;

    def.family = BLOCK_FAMILY_ID_NULL;
    def.is_stem = false;

    BlockDefReader reader(L, def_idx, proto_idx);

    if(!parse_definition(reader, ctx, def)) {
        return false;
    }

    // a block that resolves into variants is allowed to omit model_name at
    // the top level - each variants[].overrides is expected to supply its
    // own (eg. a slab's base registration has no sensible single model)
    auto has_variants = reader.try_push("variants");

    if(has_variants) {
        lua_pop(L, 1);
    }

    if(!has_variants && def.render != BLOCK_RENDER_NONE && def.model_name.is_empty()) {
        lua_pushfstring(L, "blocks.add: [%s]: model_name is required unless render is blocks.RENDER_NONE", id.full_string().data());
        return false;
    }

    auto has_checked_key = has_variants;
    auto has_states = reader.try_push("states");

    if(has_states) {
        has_checked_key = true;
        lua_pop(L, 1);
    }

    for(const auto& key : std::array { "variants", "on_tick", "on_place", "on_break", "on_interact" }) {
        if(reader.try_push(key)) {
            has_checked_key = true;
            lua_pop(L, 1);
            break;
        }
    }

    def.is_stem = has_states;

    auto block_id = ctx->register_block(id, def);

    if(has_checked_key) {
        BlockFamily family {};
        family.name = id;
        family.stem_id = block_id;
        family.default_variant = BLOCK_ID_NULL;

        if(reader.try_push("states")) {
            auto states_ok = parse_states(L, lua_gettop(L), family);
            lua_pop(L, 1);

            if(!states_ok) {
                return false;
            }
        }

        if(reader.try_push("variants")) {
            auto variants_ok = parse_variants(L, lua_gettop(L), ctx, family);
            lua_pop(L, 1);

            if(!variants_ok) {
                return false;
            }
        }

        family.on_place = parse_callback(L, def_idx, "on_place", ctx);
        family.on_break = parse_callback(L, def_idx, "on_break", ctx);
        family.on_interact = parse_callback(L, def_idx, "on_interact", ctx);
        family.on_tick = parse_callback(L, def_idx, "on_tick", ctx);

        auto family_id = ctx->register_block_family(std::move(family));
        ctx->set_block_family(block_id, family_id);
    }

    out_block_id = block_id;
    return true;
}

static int api_add(lua_State* L)
{
    auto ctx = static_cast<ModContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto argc = lua_gettop(L);
    auto raw_name = luaL_checkstring(L, 1);

    int proto_idx = 0;
    int def_idx = 2;

    if(argc >= 3) {
        proto_idx = 2;
        def_idx = 3;

        luaL_checktype(L, proto_idx, LUA_TTABLE);
    }

    luaL_checktype(L, def_idx, LUA_TTABLE);

    block_id_type block_id = BLOCK_ID_NULL;

    if(!add_block(L, ctx, raw_name, def_idx, proto_idx, block_id)) {
        return lua_error(L);
    }

    lua_pushinteger(L, block_id);
    return 1;
}

void api::open_blocks_library(std::shared_ptr<lua_State>& lua, ModContext* ctx)
{
    assert(lua);
    assert(ctx);

    auto L = lua.get();

    lua_newtable(L); // blocks

    lua_pushinteger(L, BLOCK_RENDER_SOLID);
    lua_setfield(L, -2, "RENDER_SOLID");
    lua_pushinteger(L, BLOCK_RENDER_ALPHA);
    lua_setfield(L, -2, "RENDER_ALPHA");
    lua_pushinteger(L, BLOCK_RENDER_NONE);
    lua_setfield(L, -2, "RENDER_NONE");

    lua_pushinteger(L, BLOCK_FACE_NORTH);
    lua_setfield(L, -2, "FACE_NORTH");
    lua_pushinteger(L, BLOCK_FACE_SOUTH);
    lua_setfield(L, -2, "FACE_SOUTH");
    lua_pushinteger(L, BLOCK_FACE_EAST);
    lua_setfield(L, -2, "FACE_EAST");
    lua_pushinteger(L, BLOCK_FACE_WEST);
    lua_setfield(L, -2, "FACE_WEST");
    lua_pushinteger(L, BLOCK_FACE_TOP);
    lua_setfield(L, -2, "FACE_TOP");
    lua_pushinteger(L, BLOCK_FACE_BOTTOM);
    lua_setfield(L, -2, "FACE_BOTTOM");
    lua_pushinteger(L, BLOCK_FACE_TOP);
    lua_setfield(L, -2, "FACE_UP");
    lua_pushinteger(L, BLOCK_FACE_BOTTOM);
    lua_setfield(L, -2, "FACE_DOWN");

    lua_pushinteger(L, BLOCK_TOOL_NONE);
    lua_setfield(L, -2, "TOOL_NONE");
    lua_pushinteger(L, BLOCK_TOOL_BLADE);
    lua_setfield(L, -2, "TOOL_BLADE");
    lua_pushinteger(L, BLOCK_TOOL_XBLADE);
    lua_setfield(L, -2, "TOOL_XBLADE");
    lua_pushinteger(L, BLOCK_TOOL_AXE);
    lua_setfield(L, -2, "TOOL_AXE");
    lua_pushinteger(L, BLOCK_TOOL_HOE);
    lua_setfield(L, -2, "TOOL_HOE");
    lua_pushinteger(L, BLOCK_TOOL_SHOVEL);
    lua_setfield(L, -2, "TOOL_SHOVEL");
    lua_pushinteger(L, BLOCK_TOOL_HAMMER);
    lua_setfield(L, -2, "TOOL_HAMMER");
    lua_pushinteger(L, BLOCK_TOOL_PICK);
    lua_setfield(L, -2, "TOOL_PICK");

    lua_pushinteger(L, BLOCK_TOUCH_NONE);
    lua_setfield(L, -2, "TOUCH_NONE");
    lua_pushinteger(L, BLOCK_TOUCH_SOLID);
    lua_setfield(L, -2, "TOUCH_SOLID");
    lua_pushinteger(L, BLOCK_TOUCH_BOUNCE);
    lua_setfield(L, -2, "TOUCH_BOUNCE");
    lua_pushinteger(L, BLOCK_TOUCH_THROUGH);
    lua_setfield(L, -2, "TOUCH_THROUGH");

    lua_pushinteger(L, BLOCK_TAG_GAS);
    lua_setfield(L, -2, "TAG_GAS");
    lua_pushinteger(L, BLOCK_TAG_ROCK);
    lua_setfield(L, -2, "TAG_ROCK");
    lua_pushinteger(L, BLOCK_TAG_SOIL);
    lua_setfield(L, -2, "TAG_SOIL");
    lua_pushinteger(L, BLOCK_TAG_TURF);
    lua_setfield(L, -2, "TAG_TURF");
    lua_pushinteger(L, BLOCK_TAG_XFOIL);
    lua_setfield(L, -2, "TAG_XFOIL");
    lua_pushinteger(L, BLOCK_TAG_SFOIL);
    lua_setfield(L, -2, "TAG_SFOIL");
    lua_pushinteger(L, BLOCK_TAG_WOOD);
    lua_setfield(L, -2, "TAG_WOOD");

    lua_pushinteger(L, BLOCK_TICK_RANDOM);
    lua_setfield(L, -2, "TICK_RANDOM");
    lua_pushinteger(L, BLOCK_TICK_NEIGHBOUR);
    lua_setfield(L, -2, "TICK_NEIGHBOUR");
    lua_pushinteger(L, BLOCK_TICK_SCRIPTED);
    lua_setfield(L, -2, "TICK_SCRIPTED");

    lua_pushinteger(L, BLOCK_ID_NULL);
    lua_setfield(L, -2, "NULL_BLOCK");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_get, 1);
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, &api_has_tag);
    lua_setfield(L, -2, "has_tag");

    lua_pushcfunction(L, &api_is_replaceable);
    lua_setfield(L, -2, "is_replaceable");

    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, &api_add, 1);
    lua_setfield(L, -2, "add");

    lua_setglobal(L, "blocks");
}
