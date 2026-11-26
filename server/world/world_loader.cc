#include "server/pch.hh"

#include "server/world/world_loader.hh"

#include "core/exception.hh"
#include "core/utils/physfs.hh"

#include "shared/world/biome_registry.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world_definition.hh"

#include "server/world/world_parser.hh"

constexpr static const char* CONFIG_FILE = "worlds.yml";
constexpr static const char* DEFAULT_CONFIG_FILE = "defaults/worlds.yml";

static std::string load_default(void)
{
    std::string source;
    auto loaded = utils::read_file(DEFAULT_CONFIG_FILE, source);
    vx::throw_if_not_fmt(loaded, "{}: unable to read the default world configuration", DEFAULT_CONFIG_FILE);

    auto written = utils::write_file(CONFIG_FILE, source);
    vx::throw_if_not_fmt(written, "{}: unable to write the default world configuration", CONFIG_FILE);

    return source;
}

static ryml::Tree parse_config(const std::string& source, std::string_view where)
{
    auto filename = c4::csubstr(where.data(), where.size());

    return ryml::parse_in_arena(filename, c4::to_csubstr(source));
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
        msg::log_warning("{}: unable to read the world configuration; the default is required", path_text);

        source = load_default();
    }

    auto tree = parse_config(source, path_text);
    auto root = tree.rootref();
    auto readable = root.readable();
    vx::throw_if_not_fmt(readable, "{}: malformed YAML", path_text);

    auto is_map = root.is_map();
    vx::throw_if_not_fmt(is_map, "{}: expected a mapping of worlds", path_text);

    std::vector<std::shared_ptr<ServerWorldDefinition>> parsed;
    vx::hash_set<std::string> names;

    for(auto child : root.children()) {
        auto key = child.key();
        auto name = std::string(key.str, key.len);
        auto where = std::format("{}: {}", path_text, name);

        if(!is_valid_directory(name)) {
            msg::log_warning("{}: invalid world name; the entry is ignored", where);
            continue;
        }

        if(!child.is_map()) {
            msg::log_warning("{}: must be a mapping; the entry is ignored", where);
            continue;
        }

        if(names.contains(name)) {
            msg::log_warning("{}: duplicate world name; the entry is ignored", where);
            continue;
        }

        names.emplace(name);

        auto item = world_parser::parse_world(name, child, where);

        if(item == nullptr) {
            continue;
        }

        parsed.push_back(std::move(item));
    }

    auto parsed_empty = parsed.empty();
    vx::throw_if_fmt(parsed_empty, "{}: no worlds are defined", path_text);

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

    auto result_empty = result.empty();
    vx::throw_if_fmt(result_empty, "{}: no worlds could be loaded", path_text);

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
