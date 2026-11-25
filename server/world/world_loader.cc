#include "server/pch.hh"

#include "server/world/world_loader.hh"

#include "core/exception.hh"
#include "core/utils/physfs.hh"

#include "shared/world/biome_registry.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world_definition.hh"

#include "server/world/world_json.hh"

constexpr static const char* CONFIG_FILE = "worlds.jsonc";

constexpr static std::string_view DEFAULT_CONFIG = R"jsonc(
// Voxelius world definitions. This file is JSON with comments.
//
// Each key is a world name: it is also the directory under save/ that holds
// the world, and the terrain seed is derived from the name and the universe
// seed in save/universe.conf. Rename a world only by renaming the key and its
// directory together, otherwise it starts from scratch. Exactly one world
// should set default to true; without one the first world is used.
//
// Unknown keys are ignored. An invalid value logs a warning and falls back to
// the default. Biome and weather names resolve in the builtin namespace, so
// surface_plains means builtin:surface_plains.

{
  "overworld": {
    "default": true,
    "generator": "vox17",

    "min_y": -64,
    "max_y": 255,
    "spawn": [0, 32, 0],

    "weather": ["clear", "overcast", "rain", "thunderstorm"],

    "visuals": {
      "sky": {
        "day": {
          "horizon": [0.64, 0.76, 0.94],
          "zenith": [0.26, 0.46, 0.85]
        },
        "night": {
          "horizon": [0.020, 0.030, 0.060],
          "zenith": [0.004, 0.007, 0.018]
        },
        "twilight_height": 0.35
      },
      "sun": {
        "tilt": 0.0
      },
      "moon": {
        "color": [0.86, 0.90, 1.0]
      },
      "stars": {
        "count": 5120
      },
      "clouds": {}
    },

    "biome_selector": {
      "strategy": "nearest",
      "axes": ["temperature", "humidity", "continentalness", "erosion", "weirdness"],
      "weights": [1.0, 1.0, 0.75, 1.0, 1.5],
      "entries": [
        { "biome": "surface_beach",         "target": [60, 60, 35, 80, 50] },
        { "biome": "surface_desert",        "target": [80, 20, 60, 65, 50] },
        { "biome": "surface_forest_oak",    "target": [50, 70, 55, 60, 45] },
        { "biome": "surface_forest_pine",   "target": [25, 55, 60, 50, 55] },
        { "biome": "surface_hills_grassy",  "target": [40, 50, 70, 25, 50] },
        { "biome": "surface_hills_rocky",   "target": [30, 40, 75, 15, 50] },
        { "biome": "surface_plains",        "target": [50, 55, 55, 70, 50] }
      ]
    }
  },

  "superflat": {
    "generator": "superflat",

    "spawn": [0, 1, 0],

    "weather": ["clear", "overcast", "rain", "thunderstorm"],

    "biome_selector": {
      "strategy": "constant",
      "biome": "surface_plains"
    }
  }
}
)jsonc";

static void write_default(void)
{
    auto written = utils::write_file(CONFIG_FILE, DEFAULT_CONFIG);

    if(!written) {
        msg::log_warning("{}: unable to write the default world configuration", CONFIG_FILE);
    }
}

static bool is_valid_directory(std::string_view value)
{
    if(value.empty() || value == "." || value == "..") {
        return false;
    }

    for(auto character : value) {
        auto allowed = false;
        allowed = allowed || std::isalnum(static_cast<unsigned char>(character));
        allowed = allowed || character == '_';
        allowed = allowed || character == '-';
        allowed = allowed || character == '.';

        if(!allowed) {
            return false;
        }
    }

    return true;
}

static bool resolve_biomes(WorldDefinition& definition, std::string_view where)
{
    auto& selector = definition.biome_selector;

    if(selector.strategy == BIOME_SELECTION_CONSTANT) {
        auto biome = biome_registry::find(selector.constant_biome_name);

        if(biome == BIOME_ID_NULL) {
            msg::log_error("{}: unknown biome {}; the world is skipped", where, selector.constant_biome_name.full_string());
            return false;
        }

        selector.constant_biome = biome;

        return true;
    }

    if(selector.strategy == BIOME_SELECTION_GRID) {
        selector.lookup.clear();
        selector.lookup.reserve(selector.lookup_names.size());

        for(const auto& name : selector.lookup_names) {
            auto biome = biome_registry::find(name);

            if(biome == BIOME_ID_NULL) {
                msg::log_error("{}: unknown biome {}; the world is skipped", where, name.full_string());
                return false;
            }

            selector.lookup.push_back(biome);
        }

        selector.finalize();

        return true;
    }

    std::erase_if(selector.entries, [&](BiomeSelector::Entry& entry) {
        auto biome = biome_registry::find(entry.biome_name);

        if(biome == BIOME_ID_NULL) {
            msg::log_warning("{}: unknown biome {}; the entry is ignored", where, entry.biome_name.full_string());
            return true;
        }

        entry.biome = biome;

        return false;
    });

    if(selector.entries.empty()) {
        msg::log_error("{}: a nearest biome selector has no usable entries; the world is skipped", where);
        return false;
    }

    selector.finalize();

    return true;
}

static void resolve_weather(WorldDefinition& definition, std::string_view where)
{
    definition.weather.clear();

    for(const auto& name : definition.weather_names) {
        auto weather = weather_registry::find(name);

        if(weather == WEATHER_ID_NULL) {
            msg::log_warning("{}: unknown weather {}; the entry is ignored", where, name.full_string());
            continue;
        }

        definition.weather.push_back(weather);
    }
}

std::vector<std::shared_ptr<ServerWorldDefinition>> world_loader::load(void)
{
    auto path_text = std::string(CONFIG_FILE);
    std::string source;

    if(!utils::read_file(CONFIG_FILE, source)) {
        msg::log_warning("{}: unable to read the world configuration; the default is used", path_text);
        write_default();

        source = std::string(DEFAULT_CONFIG);
    }

    auto value = json_parse_string_with_comments(source.c_str());

    if(value == nullptr) {
        msg::log_error("{}: malformed JSON; the default is used", path_text);
        write_default();

        source = std::string(DEFAULT_CONFIG);
        value = json_parse_string_with_comments(source.c_str());
    }

    vx::throw_if_not_fmt(value, "{}: malformed JSON", path_text);

    auto document = std::unique_ptr<JSON_Value, decltype(&json_value_free)>(value, &json_value_free);
    auto root = json_value_get_object(value);

    vx::throw_if_not_fmt(root, "{}: expected a JSON object", path_text);

    auto count = json_object_get_count(root);
    vx::throw_if_fmt(count == 0, "{}: no worlds are defined", path_text);

    std::vector<std::shared_ptr<ServerWorldDefinition>> parsed;
    parsed.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto name = json_object_get_name(root, i);
        auto where = std::format("{}: {}", path_text, name);

        if(!is_valid_directory(name)) {
            msg::log_warning("{}: invalid world name; the entry is ignored", where);
            continue;
        }

        auto value = json_object_get_value_at(root, i);
        auto object = json_value_get_object(value);

        if(object == nullptr) {
            msg::log_warning("{}: must be an object; the entry is ignored", where);
            continue;
        }

        auto item = world_json::parse_world(name, object, where);

        if(item == nullptr) {
            continue;
        }

        parsed.push_back(std::move(item));
    }

    // No parsed world keeps a JSON pointer, so the document can be released here
    document.reset();

    vx::throw_if_fmt(parsed.empty(), "{}: no worlds are defined", path_text);

    auto marked_default = false;

    for(std::size_t i = 0; i < parsed.size(); ++i) {
        if(!parsed[i]->is_default) {
            continue;
        }

        if(!marked_default) {
            marked_default = true;
            continue;
        }

        msg::log_warning("{}: {} is also marked as default; the mark is ignored", path_text, parsed[i]->name);

        parsed[i]->is_default = false;
    }

    if(!marked_default) {
        auto& first = parsed.front();

        msg::log_warning("{}: no world is marked as default; {} is used", path_text, first->name);

        first->is_default = true;
    }

    std::vector<std::shared_ptr<ServerWorldDefinition>> result;
    result.reserve(parsed.size());

    for(auto& item : parsed) {
        auto where = std::format("{}: {}", path_text, item->name);

        if(!resolve_biomes(*item, where)) {
            continue;
        }

        resolve_weather(*item, where);

        result.push_back(std::move(item));
    }

    vx::throw_if_fmt(result.empty(), "{}: no worlds could be loaded", path_text);

    auto has_default = std::any_of(result.begin(), result.end(), [](const std::shared_ptr<ServerWorldDefinition>& item) {
        return item->is_default;
    });

    if(!has_default) {
        auto& first = result.front();

        msg::log_warning("{}: the default world was skipped; {} is used", path_text, first->name);

        first->is_default = true;
    }

    return result;
}
