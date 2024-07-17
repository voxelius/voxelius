// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <physfs.h>
#include <shared/config.hh>
#include <shared/strtools.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>

using VariableType = unsigned short;
constexpr static VariableType VAR_STD_STRING    = 0x0000;
constexpr static VariableType VAR_SIGNED_INT    = 0x0001;
constexpr static VariableType VAR_UNSIGNED_INT  = 0x0002;
constexpr static VariableType VAR_BOOLEAN       = 0x0003;
constexpr static VariableType VAR_DOUBLE        = 0x0004;

struct Variable final {
    VariableType type {};
    void *data {};
};

static emhash8::HashMap<std::string, Variable> variables = {};

void config::add(const std::string &name, int &vref)
{
    Variable variable = {};
    variable.type = VAR_SIGNED_INT;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, bool &vref)
{
    Variable variable = {};
    variable.type = VAR_BOOLEAN;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, double &vref)
{
    Variable variable = {};
    variable.type = VAR_DOUBLE;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, unsigned int &vref)
{
    Variable variable = {};
    variable.type = VAR_UNSIGNED_INT;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, std::string &vref)
{
    Variable variable = {};
    variable.type = VAR_STD_STRING;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
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

    std::istringstream iss = std::istringstream(source);
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
        if(it->second.data == nullptr)
            continue;

        switch(it->second.type) {
            case VAR_STD_STRING:
                reinterpret_cast<std::string *>(it->second.data)[0].assign(value);
                break;
            case VAR_SIGNED_INT:
                reinterpret_cast<int *>(it->second.data)[0]
                    = static_cast<int>(strtol(value.c_str(), nullptr, 10));
                break;
            case VAR_UNSIGNED_INT:
                reinterpret_cast<unsigned int *>(it->second.data)[0]
                    = static_cast<unsigned int>(strtoul(value.c_str(), nullptr, 10));
                break;
            case VAR_BOOLEAN:
                reinterpret_cast<bool *>(it->second.data)[0]
                    = value.compare("false") && !value.compare("true");
                break;
            case VAR_DOUBLE:
                reinterpret_cast<double *>(it->second.data)[0]
                    = strtod(value.c_str(), nullptr);
                break;
        }
    }
}

void config::save(const std::string &path)
{
    std::ostringstream oss = {};

    for(const auto it : variables) {
        if(it.second.data == nullptr)
            continue;

        switch(it.second.type) {
            case VAR_STD_STRING:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<std::string *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VAR_SIGNED_INT:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<int *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VAR_UNSIGNED_INT:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<unsigned int *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VAR_BOOLEAN:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<bool *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VAR_DOUBLE:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<double *>(it.second.data)[0]);
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
