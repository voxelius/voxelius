// SPDX-License-Identifier: Zlib
#include <emhash/hash_table8.hpp>
#include <physfs.h>
#include <shared/config.hh>
#include <shared/strtools.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>

enum class VariableType {
    STRING      = 0x0000U,
    INTEGER     = 0x0001U,
    BOOL        = 0x0002U,
    DOUBLE      = 0x0003U,
    UINT        = 0x0004U,
};

struct Variable final {
    VariableType type {};
    void *ptr {nullptr};
};

static emhash8::HashMap<std::string, Variable> variables = {};

void config::add(const std::string &name, int &vref)
{
    Variable variable = {};
    variable.type = VariableType::INTEGER;
    variable.ptr = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, bool &vref)
{
    Variable variable = {};
    variable.type = VariableType::BOOL;
    variable.ptr = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, double &vref)
{
    Variable variable = {};
    variable.type = VariableType::DOUBLE;
    variable.ptr = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, unsigned int &vref)
{
    Variable variable = {};
    variable.type = VariableType::UINT;
    variable.ptr = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, std::string &vref)
{
    Variable variable = {};
    variable.type = VariableType::STRING;
    variable.ptr = &vref;

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
            case VariableType::STRING:
                reinterpret_cast<std::string *>(it->second.ptr)[0].assign(value);
                break;
            case VariableType::INTEGER:
                reinterpret_cast<int *>(it->second.ptr)[0] = static_cast<int>(strtol(value.c_str(), nullptr, 10));
                break;
            case VariableType::BOOL:
                reinterpret_cast<bool *>(it->second.ptr)[0] = value.compare("false") && !value.compare("true");
                break;
            case VariableType::DOUBLE:
                reinterpret_cast<double *>(it->second.ptr)[0] = strtod(value.c_str(), nullptr);
                break;
            case VariableType::UINT:
                reinterpret_cast<unsigned int *>(it->second.ptr)[0] = static_cast<unsigned int>(strtoul(value.c_str(), nullptr, 10));
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
            case VariableType::STRING:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<std::string *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case VariableType::INTEGER:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<int *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case VariableType::BOOL:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<bool *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case VariableType::DOUBLE:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<double *>(it.second.ptr)[0]);
                oss << std::endl;
                break;
            case VariableType::UINT:
                oss << fmt::format("{} = {}", it.first, reinterpret_cast<unsigned int *>(it.second.ptr)[0]);
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
