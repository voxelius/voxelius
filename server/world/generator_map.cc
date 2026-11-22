#include "server/pch.hh"

#include "server/world/generator_map.hh"

static vx::hash_map<std::string, GeneratorDefinition> s_definitions;

void generator_map::detail::add(std::string_view name, GeneratorDefinition definition)
{
    s_definitions[std::string(name)] = definition;
}

const GeneratorDefinition* generator_map::find(std::string_view name)
{
    auto it = s_definitions.find(std::string(name));

    if(it == s_definitions.end()) {
        return nullptr;
    }

    return &it->second;
}
