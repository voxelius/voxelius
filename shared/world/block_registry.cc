#include "shared/pch.hh"

#include "shared/world/block_registry.hh"

#include "core/exception.hh"
#include "core/utils/crc64.hh"
#include "core/utils/string.hh"

#include "shared/mod_context.hh"
#include "shared/world/fluid_registry.hh"

struct StateFluidKey final {
    bool operator==(const StateFluidKey& other) const = default;

    block_id_type id;
    blockstate_key_type state;
    blockstate_val_type value;
};

struct StateFluidKeyHash final {
    std::size_t operator()(const StateFluidKey& key) const
    {
        auto h = std::hash<std::uint64_t> {}(key.id);
        h ^= std::hash<std::uint64_t> {}(key.state) + UINT64_C(0x9E3779B97F4A7C15) + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint64_t> {}(key.value) + UINT64_C(0x9E3779B97F4A7C15) + (h << 6) + (h >> 2);
        return h;
    }
};

static std::vector<BlockDefinition> s_definitions;
static std::vector<BlockFamily> s_families;
static vx::hash_map<Identifier, block_id_type> s_names;
static vx::hash_map<block_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

static std::unordered_map<StateFluidKey, fluid_id_type, StateFluidKeyHash> s_state_fluid_cache;

static std::uint64_t hash_state_map(const vx::hash_map<blockstate_key_type, blockstate_val_type>& map)
{
    std::vector<std::pair<blockstate_key_type, blockstate_val_type>> sorted;
    sorted.reserve(map.size());

    for(const auto& it : map) {
        sorted.emplace_back(it.first, it.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::uint64_t hash = 0;

    for(const auto& it : sorted) {
        hash = utils::crc64(&it.first, sizeof(it.first), hash);
        hash = utils::crc64(&it.second, sizeof(it.second), hash);
    }

    return hash;
}

static void resolve_fluid_binding(BlockDefinition& def)
{
    if(def.fluid_name.has_value()) {
        def.fluid = fluid_registry::find(def.fluid_name.value());

        if(def.fluid == FLUID_ID_NULL) {
            msg::log_warning("unknown fluid: {}", def.fluid_name->full_string());
            def.fluid_name.reset();
            return;
        }

        def.fluid_name.reset();

        // A variant that only names a fluid renders that fluid at its full height
        if(def.fluid_level == 0) {
            if(auto fluid_def = fluid_registry::find_definition(def.fluid)) {
                def.fluid_level = fluid_def->full_level;
            }
        }
    }
}

static const BlockVariantRule* find_matching_rule(const BlockFamily* family,
    const vx::hash_map<blockstate_key_type, blockstate_val_type>& map)
{
    for(const auto& rule : family->variants) {
        auto matches = true;

        for(const auto& it : rule.when) {
            auto jt = map.find(it.first);

            if(jt == map.cend()) {
                matches = false;
                break;
            }

            if(jt->second == it.second) {
                continue;
            }

            matches = false;
            break;
        }

        if(matches) {
            return &rule;
        }
    }

    return nullptr;
}

static BlockDefinition apply_matching_variant(const BlockDefinition* base_def, const BlockFamily* family,
    const vx::hash_map<blockstate_key_type, blockstate_val_type>& map)
{
    BlockDefinition resolved = *base_def;

    if(const auto rule = find_matching_rule(family, map)) {
        resolved = BlockOverridePatch::apply(*base_def, rule->overrides);
    }

    return resolved;
}

static void hash_trivial(std::uint64_t& hash, const void* data, std::size_t size)
{
    hash = utils::crc64(data, size, hash);
}

template<typename T>
static void hash_value(std::uint64_t& hash, const T& value)
{
    hash_trivial(hash, &value, sizeof(value));
}

static void hash_text(std::uint64_t& hash, std::string_view text)
{
    hash_value(hash, static_cast<std::uint64_t>(text.size()));
    hash_trivial(hash, text.data(), text.size());
}

static void update_checksum(void)
{
    auto hash = std::uint64_t { 0 };

    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];
        auto name = s_reverse_names.find(static_cast<block_id_type>(i));

        if(name != s_reverse_names.cend()) {
            hash_text(hash, name->second.full_string());
        }

        hash_value(hash, static_cast<std::uint32_t>(def.render));

        std::vector<std::pair<std::string, std::vector<Identifier>>> albedo(def.albedo.begin(), def.albedo.end());

        std::sort(albedo.begin(), albedo.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        hash_value(hash, static_cast<std::uint64_t>(albedo.size()));

        for(const auto& entry : albedo) {
            hash_text(hash, entry.first);
            hash_value(hash, static_cast<std::uint64_t>(entry.second.size()));

            for(const auto& id : entry.second) {
                hash_text(hash, id.full_string());
            }
        }

        std::vector<std::pair<std::string, Identifier>> masks(def.masks.begin(), def.masks.end());

        std::sort(masks.begin(), masks.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        hash_value(hash, static_cast<std::uint64_t>(masks.size()));

        for(const auto& entry : masks) {
            hash_text(hash, entry.first);
            hash_text(hash, entry.second.full_string());
        }

        hash_value(hash, static_cast<std::uint8_t>(def.animated));

        hash_text(hash, def.model_name.full_string());
        hash_value(hash, def.model_offset);
        hash_value(hash, static_cast<std::uint32_t>(def.model_facing));

        hash_text(hash, def.bcoll_name.full_string());
        hash_value(hash, def.bcoll_offset);
        hash_value(hash, static_cast<std::uint32_t>(def.bcoll_facing));

        hash_value(hash, static_cast<std::uint32_t>(def.fluid));
        hash_value(hash, static_cast<std::uint32_t>(def.fluid_level));
        hash_value(hash, def.fluid_faces);
        hash_value(hash, static_cast<std::uint8_t>(def.fluid_name.has_value()));

        if(def.fluid_name) {
            hash_text(hash, def.fluid_name->full_string());
        }

        hash_value(hash, static_cast<std::uint32_t>(def.health));
        hash_value(hash, static_cast<std::uint32_t>(def.tools));
        hash_text(hash, def.sound_set.full_string());
        hash_value(hash, static_cast<std::uint8_t>(def.emission));
        hash_value(hash, static_cast<std::uint8_t>(def.dissipation));
        hash_value(hash, def.collidable);
        hash_value(hash, def.selectable);
        hash_value(hash, def.surface.friction);
        hash_value(hash, def.surface.speed);
        hash_value(hash, def.surface.bounce);
        hash_value(hash, static_cast<std::uint32_t>(def.tags));
        hash_value(hash, static_cast<std::uint8_t>(def.replaceable));

        hash_value(hash, static_cast<std::uint64_t>(def.drops.size()));

        for(const auto& drop : def.drops) {
            hash_value(hash, static_cast<std::uint64_t>(drop.items.size()));

            for(const auto& item : drop.items) {
                hash_text(hash, item.name.full_string());
                hash_value(hash, static_cast<std::uint32_t>(item.count));
            }

            hash_value(hash, static_cast<std::uint64_t>(drop.cond_effects.size()));

            for(const auto& effect : drop.cond_effects) {
                hash_text(hash, effect.full_string());
            }

            hash_value(hash, static_cast<std::uint32_t>(drop.cond_tool_bits));
        }

        hash_value(hash, static_cast<std::uint32_t>(def.family));
        hash_value(hash, static_cast<std::uint8_t>(def.is_stem));
    }

    s_checksum = hash;
}

BlockDefinition BlockOverridePatch::apply(BlockDefinition base, const BlockOverridePatch& patch)
{
    if(patch.render) {
        base.render = patch.render.value();
    }

    if(patch.albedo) {
        base.albedo = patch.albedo.value();
    }

    if(patch.masks) {
        base.masks = patch.masks.value();
    }

    if(patch.animated) {
        base.animated = patch.animated.value();
    }

    if(patch.model_name) {
        base.model_name = patch.model_name.value();
    }

    if(patch.model_offset) {
        base.model_offset = patch.model_offset.value();
    }

    if(patch.model_facing) {
        base.model_facing = patch.model_facing.value();
    }

    if(patch.bcoll_name) {
        base.bcoll_name = patch.bcoll_name.value();
    }

    if(patch.bcoll_offset) {
        base.bcoll_offset = patch.bcoll_offset.value();
    }

    if(patch.bcoll_facing) {
        base.bcoll_facing = patch.bcoll_facing.value();
    }

    if(patch.fluid_name) {
        base.fluid_name = patch.fluid_name;
    }

    if(patch.fluid_level.has_value()) {
        base.fluid_level = patch.fluid_level.value();
    }

    if(patch.fluid_faces.has_value()) {
        base.fluid_faces = patch.fluid_faces.value();
    }

    if(patch.health) {
        base.health = patch.health.value();
    }

    if(patch.sound_set) {
        base.sound_set = patch.sound_set.value();
    }

    if(patch.emission) {
        base.emission = patch.emission.value();
    }

    if(patch.dissipation) {
        base.dissipation = patch.dissipation.value();
    }

    if(patch.collidable) {
        base.collidable = patch.collidable.value();
    }

    if(patch.selectable) {
        base.selectable = patch.selectable.value();
    }

    if(patch.friction) {
        base.surface.friction = patch.friction.value();
    }

    if(patch.speed) {
        base.surface.speed = patch.speed.value();
    }

    if(patch.bounce) {
        base.surface.bounce = patch.bounce.value();
    }

    if(patch.tags) {
        base.tags = patch.tags.value();
    }

    if(patch.replaceable.has_value()) {
        base.replaceable = patch.replaceable.value();
    }

    if(patch.drops) {
        base.drops = *patch.drops;
        base.tools = BLOCK_TOOL_NONE;

        for(const auto& drop : base.drops) {
            base.tools = static_cast<block_tool_bit>(base.tools | drop.cond_tool_bits);
        }
    }

    return base;
}

blockstate_val_type BlockFamily::state_hash(std::string_view string)
{
    auto hash = static_cast<blockstate_val_type>(utils::crc64(string.data(), string.size()));
    state_values.insert_or_assign(hash, std::string(string));
    return hash;
}

std::string_view BlockFamily::state_value(blockstate_val_type value)
{
    auto it = state_values.find(value);

    if(it == state_values.cend()) {
        return {};
    }

    return it->second;
}

std::span<const BlockDefinition> block_registry::all_definitions(void)
{
    return s_definitions;
}

std::span<const BlockFamily> block_registry::all_families(void)
{
    return s_families;
}

std::uint64_t block_registry::checksum(void)
{
    return s_checksum;
}

void block_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    if(s_families.empty()) {
        s_families.emplace_back();
    }

    auto blocks = ctx.take_blocks();
    auto families = ctx.take_block_families();
    auto names = ctx.take_block_names();

    block_id_type block_offset;
    block_family_id_type family_offset;

    if(blocks.empty()) {
        block_offset = static_cast<block_id_type>(s_definitions.size());
    }
    else {
        block_offset = static_cast<block_id_type>(s_definitions.size()) - 1;
    }

    if(families.empty()) {
        family_offset = static_cast<block_family_id_type>(s_families.size());
    }
    else {
        family_offset = static_cast<block_family_id_type>(s_families.size()) - 1;
    }

    for(auto& def : blocks) {
        resolve_fluid_binding(def);
    }

    for(auto& def : blocks) {
        if(def.family) {
            def.family += family_offset;
        }
    }

    for(auto& family : families) {
        if(family.stem_id) {
            family.stem_id += block_offset;
        }

        if(family.default_variant) {
            family.default_variant += block_offset;
        }

        for(auto& it : family.resolved_states) {
            it.second += block_offset;
        }

        if(block_offset) {
            vx::hash_map<block_id_type, vx::hash_map<blockstate_key_type, blockstate_val_type>> rebased;

            for(auto& it : family.id_states) {
                rebased.try_emplace(it.first + block_offset, std::move(it.second));
            }

            family.id_states = std::move(rebased);
        }
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + block_offset;
        auto result = s_names.try_emplace(name, global_id);

        if(!result.second) {
            msg::log_warning("duplicate block name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(blocks.size()) {
        s_definitions.insert(s_definitions.end(), std::make_move_iterator(blocks.begin() + 1), std::make_move_iterator(blocks.end()));
    }

    if(families.size()) {
        s_families.insert(s_families.end(), std::make_move_iterator(families.begin() + 1), std::make_move_iterator(families.end()));

        for(auto i = family_offset; i < s_families.size(); ++i) {
            auto& family = s_families[i];

            family.default_variant = family.stem_id;

            if(family.states.size()) {
                vx::hash_map<blockstate_key_type, blockstate_val_type> default_map;

                for(const auto& it : family.states) {
                    default_map.try_emplace(it.first, it.second.default_value);
                }

                if(auto stem_def = find_definition(family.stem_id)) {
                    auto resolved = apply_matching_variant(stem_def, &family, default_map);

                    resolve_fluid_binding(resolved);

                    resolved.is_stem = false;
                    resolved.family = i;

                    auto default_id = static_cast<block_id_type>(s_definitions.size());
                    s_definitions.emplace_back(std::move(resolved));

                    family.default_variant = default_id;
                    family.resolved_states.insert_or_assign(hash_state_map(default_map), block_id_type(default_id));
                    family.id_states.insert_or_assign(block_id_type(default_id), std::move(default_map));
                }
            }

            for(const auto& rule : family.variants) {
                resolve_variant(family.stem_id, rule.when);
            }
        }
    }

    update_checksum();
}

void block_registry::purge(void)
{
    s_definitions.clear();
    s_families.clear();
    s_names.clear();
    s_reverse_names.clear();
    s_state_fluid_cache.clear();
    s_checksum = 0;
}

block_id_type block_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return BLOCK_ID_NULL;
    return it->second;
}

std::optional<Identifier> block_registry::name_of(block_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        if(auto def = find_definition(id)) {
            if(auto family = find_family(def->family)) {
                return family->name;
            }
        }

        return std::nullopt;
    }

    return it->second;
}

const BlockDefinition* block_registry::find_definition(block_id_type id)
{
    if(id == BLOCK_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const BlockDefinition* block_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}

BlockFamily* block_registry::find_family(block_family_id_type id)
{
    if(id == BLOCK_FAMILY_ID_NULL || id >= s_families.size())
        return nullptr;
    return &s_families[id];
}

BlockFamily* block_registry::find_family(const Identifier& id)
{
    for(auto& family : s_families) {
        if(family.name == id) {
            return &family;
        }
    }

    return nullptr;
}

BlockFamily* block_registry::find_family_of(block_id_type id)
{
    if(auto def = find_definition(id)) {
        return find_family(def->family);
    }

    return nullptr;
}

bool block_registry::has_tag_all(block_id_type id, block_tag_bit tag_bits)
{
    if(const auto def = find_definition(id)) {
        return tag_bits == (def->tags & tag_bits);
    }

    return false;
}

bool block_registry::has_tag_any(block_id_type id, block_tag_bit tag_bits)
{
    if(const auto def = find_definition(id)) {
        return static_cast<bool>(def->tags & tag_bits);
    }

    return false;
}

block_id_type block_registry::resolve_variant(block_id_type curr_id, const vx::hash_map<blockstate_key_type, blockstate_val_type>& map)
{
    auto def = find_definition(curr_id);

    if(def == nullptr || def->family == BLOCK_FAMILY_ID_NULL) {
        return curr_id;
    }

    auto& family = s_families[def->family];

    vx::hash_map<blockstate_key_type, blockstate_val_type> full_map;

    for(const auto& it : family.states) {
        full_map.try_emplace(it.first, it.second.default_value);
    }

    for(const auto& it : map) {
        if(family.states.contains(it.first)) {
            full_map.insert_or_assign(blockstate_key_type(it.first), blockstate_val_type(it.second));
        }
    }

    auto hash = hash_state_map(full_map);

    if(family.resolved_states.contains(hash)) {
        return family.resolved_states.at(hash);
    }

    auto stem_def = find_definition(family.stem_id);
    vx::throw_if_fmt(stem_def == nullptr, "block_registry: {}: invalid stem_id: {}", family.name.full_string(), family.stem_id);

    BlockDefinition resolved = apply_matching_variant(stem_def, &family, full_map);

    resolve_fluid_binding(resolved);

    resolved.family = def->family;
    resolved.is_stem = false;

    auto new_id = static_cast<block_id_type>(s_definitions.size());
    family.resolved_states.insert_or_assign(std::uint64_t(hash), block_id_type(new_id));
    family.id_states.insert_or_assign(block_id_type(new_id), std::move(full_map));
    s_definitions.push_back(std::move(resolved));

    return new_id;
}

fluid_id_type block_registry::preview_fluid(block_id_type id, blockstate_key_type state, blockstate_val_type value)
{
    auto def = find_definition(id);

    if(def == nullptr) {
        return FLUID_ID_NULL;
    }

    auto family = find_family(def->family);

    if(family == nullptr) {
        return FLUID_ID_NULL;
    }

    if(!family->states.contains(state)) {
        return FLUID_ID_NULL;
    }

    StateFluidKey key {};
    key.id = id;
    key.state = state;
    key.value = value;

    if(auto it = s_state_fluid_cache.find(key); it != s_state_fluid_cache.cend()) {
        return it->second;
    }

    vx::hash_map<blockstate_key_type, blockstate_val_type> map;

    if(auto it = family->id_states.find(id); it != family->id_states.cend()) {
        map = it->second;
    }

    map.insert_or_assign(blockstate_key_type(state), blockstate_val_type(value));

    fluid_id_type result = FLUID_ID_NULL;
    const auto rule = find_matching_rule(family, map);

    if(rule) {
        if(rule->overrides.fluid_name.has_value()) {
            auto fluid = fluid_registry::find(rule->overrides.fluid_name.value());

            if(rule->overrides.fluid_level.has_value()) {
                if(rule->overrides.fluid_level.value()) {
                    result = fluid;
                }
            }
            else {
                result = fluid;
            }
        }
        else if(auto stem = find_definition(family->stem_id)) {
            if(stem->fluid_level) {
                result = stem->fluid;
            }
        }
    }

    s_state_fluid_cache.insert_or_assign(key, result);

    return result;
}
