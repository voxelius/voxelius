#include "shared/pch.hh"

#include "shared/world/weather_registry.hh"

#include "core/utils/crc64.hh"

#include "shared/constant.hh"
#include "shared/mod_context.hh"
#include "shared/world/precip_registry.hh"

static std::vector<WeatherTypeDefinition> s_definitions;
static vx::hash_map<Identifier, weather_type_id> s_names;
static vx::hash_map<weather_type_id, Identifier> s_reverse_names;
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

static void update_checksum(void)
{
    std::uint64_t hash = 0;

    // Everything that changes what either side draws
    // or simulates goes in here. Hashing only the names
    // would let a client with different cloud colors
    // or a different front speed through the login check
    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];
        auto name = def.name.full_string();

        hash_trivial(hash, name.data(), name.size());

        hash_value(hash, def.weight);
        hash_value(hash, def.intensity);
        hash_value(hash, def.sky_tint);
        hash_value(hash, def.light_scale);
        hash_value(hash, def.target);
        hash_value(hash, def.strikes_per_minute);
        hash_value(hash, def.strike_radius);

        // The climate bands and visuals live in the precip registry and travel in its checksum
        auto type_count = static_cast<std::uint64_t>(def.precip.type_names.size());
        hash_value(hash, type_count);

        for(const auto& type_name : def.precip.type_names) {
            auto full = type_name.full_string();
            hash_trivial(hash, full.data(), full.size());
        }

        hash_value(hash, def.precip.intensity);

        hash_value(hash, def.cloud.height);
        hash_value(hash, def.cloud.thickness);
        hash_value(hash, def.cloud.opacity);
        hash_value(hash, def.cloud.scale);
        hash_value(hash, def.cloud.coverage);
        hash_value(hash, def.cloud.shadow_color);
        hash_value(hash, def.cloud.lit_color);
        hash_value(hash, def.cloud.seed);
    }

    s_checksum = hash;
}

std::span<const WeatherTypeDefinition> weather_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t weather_registry::checksum(void)
{
    return s_checksum;
}

void weather_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto definitions = ctx.take_weathers();
    auto names = ctx.take_weather_names();

    weather_type_id offset;

    if(definitions.empty()) {
        offset = static_cast<weather_type_id>(s_definitions.size());
    }
    else {
        offset = static_cast<weather_type_id>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + offset;
        auto result = s_names.try_emplace(name, global_id);

        if(!result.second) {
            msg::log_warning("duplicate weather name: {}", name.full_string());
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
            s_definitions[index].id = static_cast<weather_type_id>(index);
        }

        // The precip registry committed this mod's types already; a type that a later mod adds stays unknown
        for(auto index = first; index < s_definitions.size(); index += 1) {
            auto& def = s_definitions[index];
            def.precip.types.clear();

            for(const auto& type_name : def.precip.type_names) {
                auto type = precip_registry::find(type_name);

                if(type == PRECIP_ID_NULL) {
                    auto weather_name = def.name.full_string();
                    auto type_text = type_name.full_string();
                    msg::log_warning("weather {}: unknown precip type {}", weather_name, type_text);
                    continue;
                }

                def.precip.types.push_back(type);
            }
        }

        for(auto index = first; index < s_definitions.size(); index += 1) {
            for(std::size_t other = 1; other < index; other += 1) {
                if(s_definitions[index].target != s_definitions[other].target) {
                    continue;
                }

                msg::log_warning("weather {} and {} share a target; one of them will never win", s_definitions[index].name.full_string(),
                    s_definitions[other].name.full_string());
            }
        }
    }

    update_checksum();
}

void weather_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();

    s_checksum = 0;
}

weather_type_id weather_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend()) {
        return WEATHER_ID_NULL;
    }

    return it->second;
}

std::optional<Identifier> weather_registry::name_of(weather_type_id id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const WeatherTypeDefinition* weather_registry::find_definition(weather_type_id id)
{
    if(id == WEATHER_ID_NULL || id >= s_definitions.size()) {
        return nullptr;
    }

    return &s_definitions[id];
}

const WeatherTypeDefinition* weather_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}

weather_type_id weather_registry::resolve(std::string_view name)
{
    auto id = Identifier::from_string(name);

    if(id.is_valid()) {
        if(auto found = find(id)) {
            return found;
        }
    }

    if(name.find(':') != std::string_view::npos) {
        return WEATHER_ID_NULL;
    }

    auto builtin_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, name);

    if(auto found = find(builtin_id)) {
        return found;
    }

    for(const auto& def : s_definitions) {
        if(def.id && def.name.value() == name) {
            return def.id;
        }
    }

    return WEATHER_ID_NULL;
}
