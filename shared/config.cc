// SPDX-License-Identifier: Zlib
#include <emhash/hash_table8.hpp>
#include <physfs.h>
#include <shared/config.hh>
#include <shared/strtools.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>

enum class VariableType {
    STD_STRING,     // std::string
    SIGNED_INT,     // int
    UNSIGNED_INT,   // unsigned int
    BOOLEAN,        // bool
    DOUBLE,         // double
};

struct Variable final {
    VariableType type {};
    void *data {};
};

static emhash8::HashMap<std::string, Variable> variables = {};

void config::add(const std::string &name, int &vref)
{
    Variable variable = {};
    variable.type = VariableType::SIGNED_INT;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, bool &vref)
{
    Variable variable = {};
    variable.type = VariableType::BOOLEAN;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, double &vref)
{
    Variable variable = {};
    variable.type = VariableType::DOUBLE;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, unsigned int &vref)
{
    Variable variable = {};
    variable.type = VariableType::UNSIGNED_INT;
    variable.data = &vref;

    variables.insert_or_assign(name, std::move(variable));
}

void config::add(const std::string &name, std::string &vref)
{
    Variable variable = {};
    variable.type = VariableType::STD_STRING;
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
            case VariableType::STD_STRING:
                reinterpret_cast<std::string *>(it->second.data)[0].assign(value);
                break;
            case VariableType::SIGNED_INT:
                reinterpret_cast<int *>(it->second.data)[0]
                    = static_cast<int>(strtol(value.c_str(), nullptr, 10));
                break;
            case VariableType::UNSIGNED_INT:
                reinterpret_cast<unsigned int *>(it->second.data)[0]
                    = static_cast<unsigned int>(strtoul(value.c_str(), nullptr, 10));
                break;
            case VariableType::BOOLEAN:
                reinterpret_cast<bool *>(it->second.data)[0]
                    = value.compare("false") && !value.compare("true");
                break;
            case VariableType::DOUBLE:
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
            case VariableType::STD_STRING:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<std::string *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VariableType::SIGNED_INT:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<int *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VariableType::UNSIGNED_INT:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<unsigned int *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VariableType::BOOLEAN:
                oss << fmt::format("{} = {}", it.first,
                    reinterpret_cast<bool *>(it.second.data)[0]);
                oss << std::endl;
                break;
            case VariableType::DOUBLE:
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
