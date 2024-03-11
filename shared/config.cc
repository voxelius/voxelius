// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <physfs.h>
#include <shared/config.hh>
#include <shared/strtools.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

constexpr static unsigned TYPE_STRING   = 0U;
constexpr static unsigned TYPE_INT      = 1U;
constexpr static unsigned TYPE_BOOL     = 2U;
constexpr static unsigned TYPE_DOUBLE   = 3U;
constexpr static unsigned TYPE_UNSIGNED = 4U;

struct Variable final {
    unsigned type {};
    void *ptr {nullptr};
};

static std::unordered_map<std::string, Variable> variables = {};

void config::add(const std::string &name, int &vref)
{
    Variable variable = {};
    variable.type = TYPE_INT;
    variable.ptr = &vref;

    variables.insert_or_assign(name, variable);
}

void config::add(const std::string &name, bool &vref)
{
    Variable variable = {};
    variable.type = TYPE_BOOL;
    variable.ptr = &vref;

    variables.insert_or_assign(name, variable);
}

void config::add(const std::string &name, double &vref)
{
    Variable variable = {};
    variable.type = TYPE_DOUBLE;
    variable.ptr = &vref;

    variables.insert_or_assign(name, variable);
}

void config::add(const std::string &name, unsigned &vref)
{
    Variable variable = {};
    variable.type = TYPE_UNSIGNED;
    variable.ptr = &vref;

    variables.insert_or_assign(name, variable);
}

void config::add(const std::string &name, std::string &vref)
{
    Variable variable = {};
    variable.type = TYPE_STRING;
    variable.ptr = &vref;

    variables.insert_or_assign(name, variable);
}

void config::load(const std::string &path)
{
    PHYSFS_File *file = PHYSFS_openRead(path.c_str());

    if(file == nullptr)
        return;

    std::string source = {};
    source.resize(PHYSFS_fileLength(file));
    PHYSFS_readBytes(file, source.data(), source.size());
    PHYSFS_close(file);

    std::istringstream iss = std::istringstream{source};
    std::string line = {};

    while(std::getline(iss, line)) {
        const auto pos = line.find("=");

        if(pos == std::string::npos)
            continue;

        const auto name = strtools::trim_whitespace(line.substr(0, pos));
        const auto value = strtools::trim_whitespace(line.substr(pos + 1));

        const auto it = variables.find(name);

        if(it == variables.cend())
            continue;
        if(it->second.ptr == nullptr)
            continue;

        switch(it->second.type) {
            case TYPE_STRING:
                reinterpret_cast<std::string *>(it->second.ptr)[0].assign(value);
                break;
            case TYPE_INT:
                reinterpret_cast<int *>(it->second.ptr)[0] = static_cast<int>(strtol(value.c_str(), nullptr, 10));
                break;
            case TYPE_BOOL:
                reinterpret_cast<bool *>(it->second.ptr)[0] = value.compare("false") && !value.compare("true");
                break;
            case TYPE_DOUBLE:
                reinterpret_cast<double *>(it->second.ptr)[0] = strtod(value.c_str(), nullptr);
                break;
            case TYPE_UNSIGNED:
                reinterpret_cast<unsigned *>(it->second.ptr)[0] = static_cast<unsigned>(strtoul(value.c_str(), nullptr, 10));
                break;
        }
    }
}

void config::save(const std::string &path)
{
    std::ostringstream oss = {};

    for(const auto it : variables) {
        if(it.second.ptr == nullptr)
            continue;

        switch(it.second.type) {
            case TYPE_STRING:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<std::string *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case TYPE_INT:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<int *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case TYPE_BOOL:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<bool *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case TYPE_DOUBLE:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<double *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case TYPE_UNSIGNED:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<unsigned *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
        }
    }

    PHYSFS_File *file = PHYSFS_openWrite(path.c_str());

    if(file == nullptr)
        return;

    const auto source = oss.str();
    PHYSFS_writeBytes(file, source.data(), source.size());
    PHYSFS_close(file);
}
