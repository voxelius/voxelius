#include "shared/pch.hh"

#include "shared/world/biome_registry.hh"

#include "core/utils/crc64.hh"

#include "shared/mod_context.hh"
#include "shared/res/feature.hh"

static std::vector<BiomeDefinition> s_definitions;
static vx::hash_map<Identifier, biome_id_type> s_names;
static vx::hash_map<biome_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

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

static void hash_states(std::uint64_t& hash, const vx::hash_map<std::string, std::string>& states)
{
    std::vector<std::pair<std::string, std::string>> entries(states.begin(), states.end());

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    hash_value(hash, static_cast<std::uint64_t>(entries.size()));

    for(const auto& entry : entries) {
        hash_text(hash, entry.first);
        hash_text(hash, entry.second);
    }
}

static void hash_palette(std::uint64_t& hash, const BiomePaletteEntry& palette)
{
    hash_text(hash, palette.name.full_string());
    hash_states(hash, palette.states);
}

static void update_checksum(void)
{
    auto hash = std::uint64_t { 0 };

    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];
        auto name = s_reverse_names.find(static_cast<biome_id_type>(i));

        if(name != s_reverse_names.cend()) {
            hash_text(hash, name->second.full_string());
        }

        hash_value(hash, def.temperature);
        hash_value(hash, def.humidity);

        hash_palette(hash, def.palette_base);
        hash_palette(hash, def.palette_land_filler);
        hash_palette(hash, def.palette_land_surface);
        hash_palette(hash, def.palette_fluid);
        hash_palette(hash, def.palette_shore_surface);
        hash_palette(hash, def.palette_shore_filler);

        hash_value(hash, static_cast<std::uint64_t>(def.scatter.size()));

        for(const auto& entry : def.scatter) {
            hash_text(hash, entry.feature.full_string());
            hash_value(hash, entry.chance);
            hash_value(hash, static_cast<std::uint32_t>(entry.need_above));
            hash_value(hash, static_cast<std::uint32_t>(entry.need_below));
            hash_value(hash, static_cast<std::uint64_t>(entry.group_hash));
            hash_value(hash, static_cast<std::uint32_t>(entry.padding));
            hash_value(hash, static_cast<std::uint32_t>(entry.edge));
        }

        std::vector<std::pair<Identifier, Eigen::Vector3f>> tints(def.tint_map.begin(), def.tint_map.end());

        std::sort(tints.begin(), tints.end(), [](const auto& a, const auto& b) {
            return a.first.full_string() < b.first.full_string();
        });

        hash_value(hash, static_cast<std::uint64_t>(tints.size()));

        for(const auto& entry : tints) {
            hash_text(hash, entry.first.full_string());
            hash_value(hash, entry.second);
        }

        hash_value(hash, static_cast<std::uint64_t>(def.tint_colors.size()));

        for(const auto& color : def.tint_colors) {
            hash_value(hash, color);
        }
    }

    s_checksum = hash;
}

std::span<const BiomeDefinition> biome_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t biome_registry::checksum(void)
{
    return s_checksum;
}

void biome_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto biomes = ctx.take_biomes();
    auto names = ctx.take_biome_names();

    biome_id_type biome_offset;

    if(biomes.empty()) {
        biome_offset = static_cast<biome_id_type>(s_definitions.size());
    }
    else {
        biome_offset = static_cast<biome_id_type>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + biome_offset;
        auto result = s_names.try_emplace(name, global_id);

        if(!result.second) {
            msg::log_warning("duplicate biome name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(biomes.size()) {
        s_definitions.insert(s_definitions.end(), std::make_move_iterator(biomes.begin() + 1), std::make_move_iterator(biomes.end()));
    }

    update_checksum();
}

void biome_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();
    s_checksum = 0;
}

void biome_registry::resolve_palettes(void)
{
    for(auto& def : s_definitions) {
        const std::array fields = {
            &def.palette_base,
            &def.palette_land_filler,
            &def.palette_land_surface,
            &def.palette_fluid,
            &def.palette_shore_surface,
            &def.palette_shore_filler,
        };

        for(auto& field : fields) {
            auto stem = block_registry::find(field->name);
            auto family = block_registry::find_family(field->name);

            if(family == nullptr) {
                field->cached = stem;
                continue;
            }

            if(field->states.empty()) {
                field->cached = family->default_variant;
                continue;
            }

            vx::hash_map<blockstate_key_type, blockstate_val_type> map;

            for(const auto& it : field->states) {
                auto key_hash = family->state_hash(it.first);
                auto value_hash = family->state_hash(it.second);
                map.insert_or_assign(blockstate_key_type(key_hash), blockstate_val_type(value_hash));
            }

            field->cached = block_registry::resolve_variant(family->stem_id, map);
        }
    }
}

void biome_registry::resolve_features(void)
{
    for(auto& def : s_definitions) {
        for(auto& entry : def.scatter) {
            entry.cached = res::load<Feature>(entry.feature, "features", ".json");
        }
    }
}

void biome_registry::resolve_tint_colors(void)
{
    auto all_tints = tint_registry::all_definitions();

    for(auto& def : s_definitions) {
        def.tint_colors.resize(all_tints.size());

        for(std::size_t i = 1; i < all_tints.size(); ++i) {
            auto tint_id = static_cast<tint_id_type>(i);
            auto& tint_def = all_tints[i];

            auto name = tint_registry::name_of(tint_id);
            assert(name.has_value());

            auto it = def.tint_map.find(name.value());

            if(it == def.tint_map.cend()) {
                def.tint_colors[i] = tint_def.default_color;
            }
            else {
                def.tint_colors[i] = it->second;
            }
        }
    }
}

biome_id_type biome_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return BIOME_ID_NULL;
    return it->second;
}

std::optional<Identifier> biome_registry::name_of(biome_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const BiomeDefinition* biome_registry::find_definition(biome_id_type id)
{
    if(id == BIOME_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const BiomeDefinition* biome_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}
