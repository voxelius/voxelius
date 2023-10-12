// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <inttypes.h>
#include <shared/properties.hh>
#include <spdlog/fmt/fmt.h>
#include <sstream>

bool Properties::read(const vfs::path_t &path)
{
    sections.clear();

    if(vfs::file_t *file = vfs::open(path, vfs::OPEN_RD)) {
        std::string line = {};
        char section[128] = {};
        char value[512] = {};
        char key[128] = {};

        while(vfs::read_line(file, line)) {
            if(1 == sscanf(line.c_str(), " [%127[^]]] ", section))
                continue;
            if(2 != sscanf(line.c_str(), " %127[^= \t\r\n] = %511[^#\r\n]", key, value))
                continue;
            std::string value_cxx = value;
            value_cxx.erase(value_cxx.find_last_not_of(" \t\n\r\f\v") + 1);
            sections[section][key] = value_cxx;
        }

        return true;
    }

    return false;
}

bool Properties::write(const vfs::path_t &path) const
{
    std::ostringstream oss = {};

    for(const auto sit : sections) {
        oss << fmt::format("[{}]", sit.first) << std::endl;
        for(const auto it : sit.second)
            oss << fmt::format("{} = {}", it.first, it.second) << std::endl;
        oss << std::endl;
    }

    return vfs::write_string(path, oss.str());
}

bool Properties::get_bool(const std::string &section, const std::string &name, bool stub) const
{
    if(const auto sit = sections.find(section); sit != sections.cend())
        if(const auto it = sit->second.find(name); it != sit->second.cend())
            return !it->second.compare("true");
    return stub;
}

intmax_t Properties::get_int(const std::string &section, const std::string &name, intmax_t stub) const
{
    if(const auto sit = sections.find(section); sit != sections.cend())
        if(const auto it = sit->second.find(name); it != sit->second.cend())
            return strtoimax(it->second.c_str(), nullptr, 10);
    return stub;
}

double Properties::get_double(const std::string &section, const std::string &name, double stub) const
{
    if(const auto sit = sections.find(section); sit != sections.cend())
        if(const auto it = sit->second.find(name); it != sit->second.cend())
            return strtod(it->second.c_str(), nullptr);
    return stub;
}

std::string Properties::get_string(const std::string &section, const std::string &name, const std::string &stub) const
{
    if(const auto sit = sections.find(section); sit != sections.cend())
        if(const auto it = sit->second.find(name); it != sit->second.cend())
            return it->second;
    return stub;
}

void Properties::set_bool(const std::string &section, const std::string &name, bool value)
{
    sections[section][name] = value ? "true" : "false";
}

void Properties::set_int(const std::string &section, const std::string &name, intmax_t value)
{
    sections[section][name] = std::to_string(value);
}

void Properties::set_double(const std::string &section, const std::string &name, double value)
{
    sections[section][name] = std::to_string(value);
}

void Properties::set_string(const std::string &section, const std::string &name, const std::string &value)
{
    sections[section][name] = value;
}
