#include "shared/pch.hh"

#include "shared/world/weather_registry.hh"

#include "core/utils/crc64.hh"

#include "shared/constant.hh"
#include "shared/mod_context.hh"

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
    // would let a client with different cloud colours
    // or a different front speed through the login check
    for(std::size_t i = 1; i < s_definitions.size(); ++i) {
        auto& def = s_definitions[i];
        auto name = def.name.full_string();
        auto precip_kind = static_cast<std::uint8_t>(def.precipitation.kind);

        hash_trivial(hash, name.data(), name.size());

        hash_value(hash, def.weight);
        hash_value(hash, def.intensity);
        hash_value(hash, def.sky_tint);
        hash_value(hash, def.light_scale);
        hash_value(hash, def.wind_speed);
        hash_value(hash, def.strikes_per_minute);
        hash_value(hash, def.strike_radius);

        hash_value(hash, precip_kind);
        hash_value(hash, def.precipitation.intensity);
        hash_value(hash, def.precipitation.speed);
        hash_value(hash, def.precipitation.drift);
        hash_value(hash, def.precipitation.size);
        hash_value(hash, def.precipitation.color);

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
        auto [it, inserted] = s_names.try_emplace(name, global_id);

        if(!inserted) {
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
        auto found = weather_registry::find(id);

        if(found) {
            return found;
        }
    }

    if(name.find(':') != std::string_view::npos) {
        return WEATHER_ID_NULL;
    }

    auto builtin_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, name);

    if(auto found = weather_registry::find(builtin_id); found) {
        return found;
    }

    for(const auto& def : s_definitions) {
        if(def.id && def.name.value() == name) {
            return def.id;
        }
    }

    return WEATHER_ID_NULL;
}

std::optional<std::pair<float, float>> weather_registry::type_interval(std::span<const weather_type_id> allowed, weather_type_id type)
{
    auto is_allowed = [&allowed](const WeatherTypeDefinition& def) {
        if(allowed.empty()) {
            return true;
        }
        else {
            return std::ranges::contains(allowed, def.id);
        }
    };

    std::uint32_t total = 0;

    for(const auto& def : s_definitions) {
        if(def.id && is_allowed(def)) {
            total += std::max<std::uint32_t>(def.weight, 1);
        }
    }

    if(total == 0) {
        return std::nullopt;
    }

    std::uint32_t offset = 0;

    for(const auto& def : s_definitions) {
        if(def.id == WEATHER_ID_NULL || !is_allowed(def)) {
            continue;
        }

        auto weight = std::max<std::uint32_t>(def.weight, 1);

        if(def.id == type) {
            auto start = static_cast<float>(offset) / static_cast<float>(total);
            auto end = static_cast<float>(offset + weight) / static_cast<float>(total);
            return std::make_pair(start, end);
        }

        offset += weight;
    }

    return std::nullopt;
}
