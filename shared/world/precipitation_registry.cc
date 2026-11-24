#include "shared/pch.hh"

#include "shared/world/precipitation_registry.hh"

#include "core/utils/crc64.hh"

#include "shared/constant.hh"
#include "shared/mod_context.hh"

static std::vector<PrecipitationDefinition> s_definitions;
static vx::hash_map<Identifier, precipitation_type_id> s_names;
static vx::hash_map<precipitation_type_id, Identifier> s_reverse_names;
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

static void update_checksum(void)
{
    std::uint64_t hash = 0;

    // Everything that changes what either side draws travels in this
    // hash; the weather hash covers only the fronts and their candidates
    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];

        hash_text(hash, def.name.full_string());

        hash_value(hash, def.temperature_min);
        hash_value(hash, def.temperature_max);
        hash_value(hash, def.temperature_fade);
        hash_value(hash, def.humidity_min);
        hash_value(hash, def.humidity_max);
        hash_value(hash, def.humidity_fade);
        hash_value(hash, def.priority);

        auto texture = std::string();

        if(def.texture.is_valid()) {
            texture = def.texture.full_string();
        }

        hash_text(hash, texture);

        hash_value(hash, def.quad_width);
        hash_value(hash, def.band_half_height);
        hash_value(hash, def.fall_speed);
        hash_value(hash, def.draw_chance);
        hash_value(hash, def.opacity);
        hash_value(hash, def.color);
    }

    s_checksum = hash;
}

std::span<const PrecipitationDefinition> precipitation_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t precipitation_registry::checksum(void)
{
    return s_checksum;
}

void precipitation_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto definitions = ctx.take_precipitations();
    auto names = ctx.take_precipitation_names();

    precipitation_type_id offset;

    if(definitions.empty()) {
        offset = static_cast<precipitation_type_id>(s_definitions.size());
    }
    else {
        offset = static_cast<precipitation_type_id>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + offset;
        auto [it, inserted] = s_names.try_emplace(name, global_id);

        if(!inserted) {
            msg::log_warning("duplicate precipitation name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(definitions.size()) {
        auto first = s_definitions.size();

        s_definitions.insert(s_definitions.end(), std::make_move_iterator(definitions.begin() + 1),
            std::make_move_iterator(definitions.end()));

        // The definition carries its local id only; the
        // registry id is the index in the combined list
        for(auto index = first; index < s_definitions.size(); index += 1) {
            s_definitions[index].id = static_cast<precipitation_type_id>(index);
        }
    }

    update_checksum();
}

void precipitation_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();

    s_checksum = 0;
}

precipitation_type_id precipitation_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend()) {
        return PRECIPITATION_ID_NULL;
    }

    return it->second;
}

std::optional<Identifier> precipitation_registry::name_of(precipitation_type_id id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const PrecipitationDefinition* precipitation_registry::find_definition(precipitation_type_id id)
{
    if(id == PRECIPITATION_ID_NULL || id >= s_definitions.size()) {
        return nullptr;
    }

    return &s_definitions[id];
}

const PrecipitationDefinition* precipitation_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}

precipitation_type_id precipitation_registry::resolve(std::string_view name)
{
    auto id = Identifier::from_string(name);

    if(id.is_valid()) {
        auto found = find(id);

        if(found) {
            return found;
        }
    }

    if(name.find(':') != std::string_view::npos) {
        return PRECIPITATION_ID_NULL;
    }

    auto builtin_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, name);
    return find(builtin_id);
}
