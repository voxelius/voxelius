#include "server/pch.hh"

#include "server/world/world_json.hh"

#include "core/identifier.hh"
#include "core/utils/json.hh"

#include "shared/constant.hh"
#include "shared/world/world_definition.hh"

#include "server/world/generator_map.hh"

constexpr static std::size_t COLOR_COMPONENTS = 3;
constexpr static std::size_t SPAWN_COMPONENTS = 3;
constexpr static long long TARGET_MIN = 0;
constexpr static long long TARGET_MAX = 99;

static const JSON_Value* find_value(const JSON_Object* object, const char* key)
{
    return json_object_get_value(object, key);
}

static std::string join_path(std::string_view where, std::string_view key)
{
    return std::format("{}.{}", where, key);
}

static void warn_default(std::string_view where, std::string_view key, std::string_view expected)
{
    msg::log_warning("{}: {} must be {}; the default is used", where, key, expected);
}

static void warn_ignored(std::string_view where, std::string_view key, std::string_view expected)
{
    msg::log_warning("{}: {} must be {}; the value is ignored", where, key, expected);
}

static std::optional<std::string_view> parse_string(const JSON_Object* object, std::string_view where, const char* key)
{
    auto value = find_value(object, key);

    if(value == nullptr) {
        return std::nullopt;
    }

    auto text = json_value_get_string(value);

    if(text == nullptr) {
        warn_default(where, key, "a string");
        return std::nullopt;
    }

    return std::string_view(text);
}

static std::optional<double> parse_number(const JSON_Object* object, std::string_view where, const char* key)
{
    if(auto value = utils::parse_arithmetic<double>(object, key)) {
        return value;
    }

    if(json_object_has_value(object, key)) {
        warn_default(where, key, "a number");
    }

    return std::nullopt;
}

static std::optional<long long> parse_integer(const JSON_Object* object, std::string_view where, const char* key)
{
    if(auto value = utils::parse_integer<long long>(object, key)) {
        return value;
    }

    if(json_object_has_value(object, key)) {
        warn_default(where, key, "an integer");
    }

    return std::nullopt;
}

static bool parse_bool(const JSON_Object* object, std::string_view where, const char* key, bool fallback)
{
    auto value = find_value(object, key);

    if(value == nullptr) {
        return fallback;
    }

    if(JSONBoolean == json_value_get_type(value)) {
        return static_cast<bool>(json_value_get_boolean(value));
    }

    warn_default(where, key, "a boolean");

    return fallback;
}

static const JSON_Array* parse_array(const JSON_Object* object, std::string_view where, const char* key)
{
    auto value = find_value(object, key);

    if(value == nullptr) {
        return nullptr;
    }

    auto array = json_value_get_array(value);

    if(array == nullptr) {
        warn_default(where, key, "an array");
        return nullptr;
    }

    return array;
}

static const JSON_Object* parse_object(const JSON_Object* object, std::string_view where, const char* key)
{
    auto value = find_value(object, key);

    if(value == nullptr) {
        return nullptr;
    }

    auto nested = json_value_get_object(value);

    if(nested == nullptr) {
        warn_default(where, key, "an object");
        return nullptr;
    }

    return nested;
}

static std::optional<std::vector<std::string>> parse_string_list(const JSON_Object* object, std::string_view where, const char* key)
{
    auto array = parse_array(object, where, key);

    if(array == nullptr) {
        return std::nullopt;
    }

    auto count = json_array_get_count(array);

    std::vector<std::string> result;
    result.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto text = json_array_get_string(array, i);

        if(text == nullptr) {
            auto element_where = std::format("{}[{}]", key, i);

            warn_ignored(where, element_where, "a string");
            return std::nullopt;
        }

        result.emplace_back(text);
    }

    return result;
}

static std::optional<std::vector<float>> parse_number_list(const JSON_Object* object, std::string_view where, const char* key)
{
    auto array = parse_array(object, where, key);

    if(array == nullptr) {
        return std::nullopt;
    }

    auto count = json_array_get_count(array);

    std::vector<float> result;
    result.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto element_where = std::format("{}[{}]", key, i);
        auto number = utils::parse_arithmetic<float>(json_array_get_value(array, i));

        if(!number.has_value() || !std::isfinite(number.value())) {
            warn_ignored(where, element_where, "a finite number");
            return std::nullopt;
        }

        result.push_back(number.value());
    }

    return result;
}

static std::optional<std::vector<std::uint8_t>> parse_target_list(const JSON_Object* object, std::string_view where)
{
    auto array = parse_array(object, where, "target");

    if(array == nullptr) {
        return std::nullopt;
    }

    auto count = json_array_get_count(array);

    std::vector<std::uint8_t> result;
    result.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto element_where = std::format("target[{}]", i);
        auto integer = utils::parse_integer<long long>(json_array_get_value(array, i));

        if(!integer.has_value()) {
            warn_ignored(where, element_where, "an integer");
            return std::nullopt;
        }

        auto target = integer.value();

        if(target < TARGET_MIN || target > TARGET_MAX) {
            warn_ignored(where, element_where, "in range 0..99");
            return std::nullopt;
        }

        result.push_back(static_cast<std::uint8_t>(target));
    }

    return result;
}

static std::optional<Eigen::Vector3f> parse_color(const JSON_Object* object, std::string_view where, const char* key)
{
    auto array = parse_array(object, where, key);

    if(array == nullptr) {
        return std::nullopt;
    }

    if(json_array_get_count(array) != COLOR_COMPONENTS) {
        warn_ignored(where, key, "three numbers");
        return std::nullopt;
    }

    auto color = Eigen::Vector3f(0.0f, 0.0f, 0.0f);

    for(std::size_t i = 0; i < COLOR_COMPONENTS; ++i) {
        auto number = utils::parse_arithmetic<float>(json_array_get_value(array, i));

        if(!number.has_value()) {
            warn_ignored(where, key, "three numbers");
            return std::nullopt;
        }

        color[static_cast<Eigen::Index>(i)] = number.value();
    }

    return color;
}

static std::optional<BlockPos> parse_block_pos(const JSON_Object* object, std::string_view where, const char* key)
{
    auto array = parse_array(object, where, key);

    if(array == nullptr) {
        return std::nullopt;
    }

    if(json_array_get_count(array) != SPAWN_COMPONENTS) {
        warn_default(where, key, "three integers");
        return std::nullopt;
    }

    auto pos = BlockPos(0, 0, 0);

    for(std::size_t i = 0; i < SPAWN_COMPONENTS; ++i) {
        auto integer = utils::parse_integer<long long>(json_array_get_value(array, i));

        if(!integer.has_value()) {
            warn_default(where, key, "three integers");
            return std::nullopt;
        }

        pos[static_cast<Eigen::Index>(i)] = static_cast<BlockPos::value_type>(integer.value());
    }

    return pos;
}

static std::optional<WorldVisuals::Sky::Gradient> parse_gradient(const JSON_Object* object, std::string_view where)
{
    auto horizon = parse_color(object, where, "horizon");
    auto zenith = parse_color(object, where, "zenith");

    if(!horizon.has_value() || !zenith.has_value()) {
        msg::log_warning("{}: incomplete gradient; the visual is ignored", where);
        return std::nullopt;
    }

    WorldVisuals::Sky::Gradient gradient;
    gradient.horizon = horizon.value();
    gradient.zenith = zenith.value();

    return gradient;
}

static std::optional<WorldVisuals::Sky> parse_sky(const JSON_Object* object, std::string_view where)
{
    auto day = parse_object(object, where, "day");
    auto night = parse_object(object, where, "night");
    auto twilight = parse_number(object, where, "twilight_height");

    if(day == nullptr || night == nullptr || !twilight.has_value()) {
        msg::log_warning("{}: incomplete sky; the visual is ignored", where);
        return std::nullopt;
    }

    auto day_gradient = parse_gradient(day, join_path(where, "day"));
    auto night_gradient = parse_gradient(night, join_path(where, "night"));

    if(!day_gradient.has_value() || !night_gradient.has_value()) {
        return std::nullopt;
    }

    WorldVisuals::Sky sky;
    sky.day = day_gradient.value();
    sky.night = night_gradient.value();
    sky.twilight_height = static_cast<float>(twilight.value());

    return sky;
}

static std::optional<WorldVisuals::Sun> parse_sun(const JSON_Object* object, std::string_view where)
{
    auto tilt = parse_number(object, where, "tilt");

    if(!tilt.has_value()) {
        msg::log_warning("{}: sun needs a tilt; the visual is ignored", where);
        return std::nullopt;
    }

    WorldVisuals::Sun sun;
    sun.tilt = static_cast<float>(tilt.value());

    return sun;
}

static std::optional<WorldVisuals::Moon> parse_moon(const JSON_Object* object, std::string_view where)
{
    auto color = parse_color(object, where, "color");

    if(!color.has_value()) {
        msg::log_warning("{}: moon needs a color; the visual is ignored", where);
        return std::nullopt;
    }

    WorldVisuals::Moon moon;
    moon.color = color.value();

    return moon;
}

static std::optional<WorldVisuals::Stars> parse_stars(const JSON_Object* object, std::string_view where)
{
    auto count = parse_integer(object, where, "count");

    if(!count.has_value()) {
        msg::log_warning("{}: stars need a count; the visual is ignored", where);
        return std::nullopt;
    }

    if(count.value() < 0) {
        warn_ignored(where, "count", "non-negative");
        return std::nullopt;
    }

    if(count.value() > static_cast<long long>(std::numeric_limits<std::uint32_t>::max())) {
        warn_ignored(where, "count", "within the 32-bit range");
        return std::nullopt;
    }

    WorldVisuals::Stars stars;
    stars.count = static_cast<std::uint32_t>(count.value());

    return stars;
}

static bool parse_nearest_selector(const JSON_Object* object, BiomeSelector& selector, std::string_view where)
{
    auto axes = parse_string_list(object, where, "axes");
    auto weights = parse_number_list(object, where, "weights");

    if(!axes.has_value() || !weights.has_value()) {
        msg::log_error("{}: a nearest biome selector needs axes and weights; the world is skipped", where);
        return false;
    }

    if(axes->empty()) {
        msg::log_error("{}: a nearest biome selector needs at least one axis; the world is skipped", where);
        return false;
    }

    if(axes->size() != weights->size()) {
        msg::log_error("{}: axes and weights must have the same length; the world is skipped", where);
        return false;
    }

    selector.axes = std::move(axes.value());
    selector.weights = std::move(weights.value());

    auto entries = parse_array(object, where, "entries");

    if(entries == nullptr) {
        msg::log_error("{}: a nearest biome selector needs entries; the world is skipped", where);
        return false;
    }

    auto count = json_array_get_count(entries);

    for(std::size_t i = 0; i < count; ++i) {
        auto entry_where = std::format("{}.entries[{}]", where, i);
        auto entry_object = json_array_get_object(entries, i);

        if(entry_object == nullptr) {
            warn_ignored(entry_where, "the entry", "an object");
            continue;
        }

        auto biome = parse_string(entry_object, entry_where, "biome");

        if(!biome.has_value()) {
            warn_ignored(entry_where, "biome", "a string");
            continue;
        }

        auto biome_name = Identifier::from_string(biome.value(), constant::BUILTIN_NAME_SPACE);

        if(!biome_name.is_valid()) {
            warn_ignored(entry_where, "biome", "a valid identifier");
            continue;
        }

        auto target = parse_target_list(entry_object, entry_where);

        if(!target.has_value()) {
            warn_ignored(entry_where, "the entry", "a valid target");
            continue;
        }

        if(target->size() != selector.weights.size()) {
            warn_ignored(entry_where, "target", "one value per axis");
            continue;
        }

        BiomeSelector::Entry entry {};
        entry.biome_name = biome_name;
        entry.target = std::move(target.value());

        if(auto priority = parse_integer(entry_object, entry_where, "priority")) {
            if(priority.value() < 0) {
                warn_default(entry_where, "priority", "non-negative");
            }
            else {
                entry.priority = static_cast<unsigned>(priority.value());
            }
        }

        if(auto offset = parse_number(entry_object, entry_where, "offset")) {
            entry.offset = static_cast<float>(offset.value());
        }

        selector.entries.push_back(std::move(entry));
    }

    if(selector.entries.empty()) {
        msg::log_error("{}: a nearest biome selector has no usable entries; the world is skipped", where);
        return false;
    }

    return true;
}

static bool parse_grid_selector(const JSON_Object* object, BiomeSelector& selector, std::string_view where)
{
    auto axes = parse_string_list(object, where, "axes");

    if(!axes.has_value() || axes->size() != 2) {
        msg::log_error("{}: a grid biome selector needs exactly two axes; the world is skipped", where);
        return false;
    }

    auto grid_size = parse_integer(object, where, "grid_size");

    if(!grid_size.has_value()) {
        msg::log_error("{}: a grid biome selector needs grid_size; the world is skipped", where);
        return false;
    }

    if(grid_size.value() < 1 || grid_size.value() > static_cast<long long>(BIOME_SELECTOR_MAX_GRID_SIZE)) {
        msg::log_error("{}: grid_size is out of range; the world is skipped", where);
        return false;
    }

    auto size = static_cast<std::size_t>(grid_size.value());
    auto rows = parse_array(object, where, "lookup");

    if(rows == nullptr || json_array_get_count(rows) != size) {
        msg::log_error("{}: a grid biome selector needs grid_size rows; the world is skipped", where);
        return false;
    }

    selector.axes = std::move(axes.value());
    selector.grid_size = size;
    selector.lookup_names.reserve(size * size);

    for(std::size_t row = 0; row < size; row += 1) {
        auto row_where = std::format("{}.lookup[{}]", where, row);
        auto row_array = json_array_get_array(rows, row);

        if(row_array == nullptr || json_array_get_count(row_array) != size) {
            msg::log_error("{}: a grid row needs grid_size entries; the world is skipped", row_where);
            return false;
        }

        for(std::size_t column = 0; column < size; column += 1) {
            auto entry_where = std::format("{}[{}]", row_where, column);
            auto text = json_array_get_string(row_array, column);

            if(text == nullptr) {
                msg::log_error("{}: a grid entry must be a string; the world is skipped", entry_where);
                return false;
            }

            auto name = Identifier::from_string(text, constant::BUILTIN_NAME_SPACE);

            if(!name.is_valid()) {
                msg::log_error("{}: a grid entry is not a valid identifier; the world is skipped", entry_where);
                return false;
            }

            selector.lookup_names.push_back(name);
        }
    }

    return true;
}

static bool parse_selector(const JSON_Object* object, BiomeSelector& selector, std::string_view where)
{
    auto strategy = parse_string(object, where, "strategy");

    if(!strategy.has_value()) {
        msg::log_error("{}: biome_selector needs a strategy; the world is skipped", where);
        return false;
    }

    if(strategy.value() == "constant") {
        auto biome = parse_string(object, where, "biome");

        if(!biome.has_value()) {
            msg::log_error("{}: a constant biome selector needs a biome; the world is skipped", where);
            return false;
        }

        auto name = Identifier::from_string(biome.value(), constant::BUILTIN_NAME_SPACE);

        if(!name.is_valid()) {
            msg::log_error("{}: biome is not a valid identifier; the world is skipped", where);
            return false;
        }

        selector.strategy = BIOME_SELECTION_CONSTANT;
        selector.constant_biome_name = name;

        return true;
    }

    if(strategy.value() == "nearest") {
        selector.strategy = BIOME_SELECTION_NEAREST;
        return parse_nearest_selector(object, selector, where);
    }

    if(strategy.value() == "grid") {
        selector.strategy = BIOME_SELECTION_GRID;
        return parse_grid_selector(object, selector, where);
    }

    msg::log_error("{}: unknown biome selector strategy {}; the world is skipped", where, strategy.value());
    return false;
}

static void parse_visuals(const JSON_Object* object, WorldVisuals& visuals, std::string_view where)
{
    if(auto sky = parse_object(object, where, "sky")) {
        if(auto value = parse_sky(sky, join_path(where, "sky"))) {
            visuals.sky = std::move(value);
        }
    }

    if(auto sun = parse_object(object, where, "sun")) {
        if(auto value = parse_sun(sun, join_path(where, "sun"))) {
            visuals.sun = std::move(value);
        }
    }

    if(auto moon = parse_object(object, where, "moon")) {
        if(auto value = parse_moon(moon, join_path(where, "moon"))) {
            visuals.moon = std::move(value);
        }
    }

    if(auto stars = parse_object(object, where, "stars")) {
        if(auto value = parse_stars(stars, join_path(where, "stars"))) {
            visuals.stars = std::move(value);
        }
    }

    if(parse_object(object, where, "clouds")) {
        visuals.clouds = WorldVisuals::Clouds {};
    }
}

std::shared_ptr<ServerWorldDefinition> world_json::parse_world(std::string_view name, const JSON_Object* object, std::string_view where)
{
    auto generator_name = parse_string(object, where, "generator");

    if(!generator_name.has_value() || generator_name->empty()) {
        msg::log_error("{}: generator is required; the world is skipped", where);
        return nullptr;
    }

    auto selector = parse_object(object, where, "biome_selector");

    if(selector == nullptr) {
        msg::log_error("{}: biome_selector is required; the world is skipped", where);
        return nullptr;
    }

    auto result = std::make_shared<ServerWorldDefinition>();
    result->name = std::string(name);
    result->generator_name = std::string(generator_name.value());

    result->is_default = parse_bool(object, where, "default", false);

    if(auto min_y = parse_integer(object, where, "min_y")) {
        result->min_y = static_cast<BlockPos::value_type>(min_y.value());
    }

    if(auto max_y = parse_integer(object, where, "max_y")) {
        result->max_y = static_cast<BlockPos::value_type>(max_y.value());
    }

    if(result->min_y > result->max_y) {
        msg::log_warning("{}: min_y is greater than max_y; the default height is used", where);

        result->min_y = -64;
        result->max_y = 255;
    }

    result->spawn = BlockPos(0, 32, 0);

    if(auto spawn = parse_block_pos(object, where, "spawn")) {
        result->spawn = spawn.value();
    }

    if(auto weather = parse_array(object, where, "weather")) {
        auto count = json_array_get_count(weather);

        for(std::size_t i = 0; i < count; ++i) {
            auto entry_where = std::format("{}.weather[{}]", where, i);
            auto text = json_array_get_string(weather, i);

            if(text == nullptr) {
                msg::log_warning("{}: must be a string; the entry is ignored", entry_where);
                continue;
            }

            auto name = Identifier::from_string(text, constant::BUILTIN_NAME_SPACE);

            if(!name.is_valid()) {
                msg::log_warning("{}: not a valid identifier; the entry is ignored", entry_where);
                continue;
            }

            result->weather_names.push_back(name);
        }
    }

    if(auto visuals = parse_object(object, where, "visuals")) {
        parse_visuals(visuals, result->visuals, join_path(where, "visuals"));
    }

    if(!parse_selector(selector, result->biome_selector, join_path(where, "biome_selector"))) {
        return nullptr;
    }

    auto generator = generator_map::find(result->generator_name);

    if(generator == nullptr) {
        msg::log_error("{}: unknown generator {}; the world is skipped", where, result->generator_name);
        return nullptr;
    }

    auto settings = parse_object(object, where, "generator_settings");
    auto settings_where = std::format("{}.generator_settings", where);
    auto state = generator->create_state(settings, *result, settings_where);

    if(state == nullptr) {
        return nullptr;
    }

    result->generator = generator;
    result->generator_state = std::move(state);

    return result;
}
