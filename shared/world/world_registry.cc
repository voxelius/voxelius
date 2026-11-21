#include "shared/pch.hh"

#include "shared/world/biome_registry.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world_registry.hh"

#include "core/exception.hh"

static std::vector<std::shared_ptr<WorldDefinition>> s_definitions;

static bool is_valid_directory(std::string_view value)
{
    if(value.empty() || value == "." || value == "..") {
        return false;
    }

    for(auto character : value) {
        auto allowed = std::isalnum(static_cast<unsigned char>(character)) != 0;
        allowed = allowed || character == '_';
        allowed = allowed || character == '-';
        allowed = allowed || character == '.';

        if(!allowed) {
            return false;
        }
    }

    return true;
}

static bool has_directory(std::string_view directory, const WorldDefinition* except)
{
    for(const auto& definition : s_definitions) {
        if(definition.get() == except) {
            continue;
        }

        if(definition->directory == directory) {
            return true;
        }
    }

    return false;
}

std::shared_ptr<WorldDefinition> world_registry::add(const Identifier& id)
{
    if(find(id)) {
        return nullptr;
    }

    auto definition = std::make_shared<WorldDefinition>();
    definition->id = id;

    s_definitions.push_back(definition);

    return definition;
}

std::shared_ptr<WorldDefinition> world_registry::find(const Identifier& id)
{
    for(const auto& definition : s_definitions) {
        if(definition->id == id) {
            return definition;
        }
    }

    return nullptr;
}

std::span<const std::shared_ptr<WorldDefinition>> world_registry::all(void)
{
    return s_definitions;
}

void world_registry::remove(const Identifier& id)
{
    std::erase_if(s_definitions, [&](const auto& definition) {
        return definition->id == id;
    });
}

void world_registry::resolve(void)
{
    vx::throw_if_not(!s_definitions.empty(), "no world is registered");

    for(const auto& definition : s_definitions) {
        const auto& id = definition->id;
        auto full_id = id.full_string();

        if(definition->directory.empty()) {
            auto directory = std::string(full_id);
            std::replace(directory.begin(), directory.end(), ':', '_');
            definition->directory = std::move(directory);
        }

        vx::throw_if_not_fmt(is_valid_directory(definition->directory), "world {}: invalid directory \"{}\"", full_id,
            definition->directory);
        vx::throw_if_not_fmt(!has_directory(definition->directory, definition.get()), "world {}: directory \"{}\" is already taken",
            full_id, definition->directory);
        vx::throw_if_not_fmt(definition->min_y <= definition->max_y, "world {}: min_y is greater than max_y", full_id);

        auto& selector = definition->biome_selector;

        for(auto& entry : selector.entries) {
            auto biome = biome_registry::find(entry.biome_name);
            vx::throw_if_not_fmt(biome, "world {}: unknown biome {}", full_id, entry.biome_name.full_string());
            entry.biome = biome;
        }

        if(selector.strategy == BIOME_SELECTION_CONSTANT && selector.constant_biome_name.is_valid()) {
            auto biome = biome_registry::find(selector.constant_biome_name);
            vx::throw_if_not_fmt(biome, "world {}: unknown biome {}", full_id, selector.constant_biome_name.full_string());
            selector.constant_biome = biome;
        }

        if(selector.strategy == BIOME_SELECTION_GRID) {
            selector.lookup.clear();
            selector.lookup.reserve(selector.lookup_names.size());

            for(const auto& name : selector.lookup_names) {
                auto biome = biome_registry::find(name);
                vx::throw_if_not_fmt(biome, "world {}: unknown biome {}", full_id, name.full_string());
                selector.lookup.push_back(biome);
            }
        }

        selector.finalize();

        definition->weather.clear();

        for(const auto& name : definition->weather_names) {
            auto weather = weather_registry::find(name);
            vx::throw_if_not_fmt(weather, "world {}: unknown weather {}", full_id, name.full_string());
            definition->weather.push_back(weather);
        }
    }
}

void world_registry::purge(void)
{
    s_definitions.clear();
}
