// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/strtools.hh>
#include <shared/fstools.hh>
#include <shared/config.hh>
#include <spdlog/fmt/fmt.h>
#include <sstream>

void Config::add(Config &config, const std::string &name, int &vref)
{
    ConfigVariable variable = {};
    variable.type = CONFIG_INT;
    variable.value_ptr = &vref;
    config.vmap.insert_or_assign(name, std::move(variable));
}

void Config::add(Config &config, const std::string &name, bool &vref)
{
    ConfigVariable variable = {};
    variable.type = CONFIG_BOOLEAN;
    variable.value_ptr = &vref;
    config.vmap.insert_or_assign(name, std::move(variable));
}

void Config::add(Config &config, const std::string &name, float &vref)
{
    ConfigVariable variable = {};
    variable.type = CONFIG_FLOAT;
    variable.value_ptr = &vref;
    config.vmap.insert_or_assign(name, std::move(variable));
}

void Config::add(Config &config, const std::string &name, std::string &vref)
{
    ConfigVariable variable = {};
    variable.type = CONFIG_STD_STRING;
    variable.value_ptr = &vref;
    config.vmap.insert_or_assign(name, std::move(variable));
}

void Config::add(Config &config, const std::string &name, unsigned int &vref)
{
    ConfigVariable variable = {};
    variable.type = CONFIG_UNSIGNED_INT;
    variable.value_ptr = &vref;
    config.vmap.insert_or_assign(name, std::move(variable));
}

void Config::clear(Config &config)
{
    config.vmap.clear();
}

bool Config::load(Config &config, const std::string &path)
{
    std::string source = {};
    if(!fstools::read_string(path, source))
        return false;

    std::string line;
    std::istringstream stream = std::istringstream(source);

    while(std::getline(stream, line)) {
        const auto dpos = line.find('=');

        if(dpos == std::string::npos) {
            // Invalid line, just ignore it
            continue;
        }

        const auto name = strtools::trim_whitespace(line.substr(0, dpos));
        const auto value = strtools::trim_whitespace(line.substr(dpos + 1));

        const auto it = config.vmap.find(name);
        if(it == config.vmap.cend())
            continue;

        if(it->second.type == CONFIG_INT) {
            reinterpret_cast<int *>(it->second.value_ptr)[0]
                = static_cast<int>(std::strtol(value.c_str(), nullptr, 10));
            continue;
        }

        if(it->second.type == CONFIG_BOOLEAN) {
            reinterpret_cast<bool *>(it->second.value_ptr)[0] =
                value.compare("false") && !value.compare("true");
            continue;
        }

        if(it->second.type == CONFIG_FLOAT) {
            reinterpret_cast<float *>(it->second.value_ptr)[0] =
                std::strtof(value.c_str(), nullptr);
            continue;
        }

        if(it->second.type == CONFIG_STD_STRING) {
            reinterpret_cast<std::string *>(it->second.value_ptr)->assign(value);
            continue;
        }

        if(it->second.type == CONFIG_UNSIGNED_INT) {
            reinterpret_cast<unsigned int *>(it->second.value_ptr)[0] =
                static_cast<unsigned int>(std::strtoul(value.c_str(), nullptr, 10));
            continue;
        }
    }

    return true;
}

bool Config::save(const Config &config, const std::string &path)
{
    std::ostringstream stream = {};

    for(const auto it : config.vmap) {
        if(it.second.type == CONFIG_INT) {
            stream << fmt::format("{} = {}", it.first,
                reinterpret_cast<int *>(it.second.value_ptr)[0]);
            stream << std::endl;
            continue;
        }

        if(it.second.type == CONFIG_BOOLEAN) {
            stream << fmt::format("{} = {}", it.first,
                reinterpret_cast<bool *>(it.second.value_ptr)[0]);
            stream << std::endl;
            continue;
        }

        if(it.second.type == CONFIG_FLOAT) {
            stream << fmt::format("{} = {}", it.first,
                reinterpret_cast<float *>(it.second.value_ptr)[0]);
            stream << std::endl;
            continue;
        }

        if(it.second.type == CONFIG_STD_STRING) {
            stream << fmt::format("{} = {}", it.first,
                reinterpret_cast<std::string *>(it.second.value_ptr)[0]);
            stream << std::endl;
            continue;
        }

        if(it.second.type == CONFIG_UNSIGNED_INT) {
            stream << fmt::format("{} = {}", it.first,
                reinterpret_cast<unsigned int *>(it.second.value_ptr)[0]);
            stream << std::endl;
            continue;
        }
    }

    return fstools::write_string(path, stream.str());
}
