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
#include <shared/cmdline.hh>
#include <shared/config/config.hh>
#include <spdlog/fmt/fmt.h>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, config::IVariable *> map = {};

static std::string trim_whitespace(const std::string &s)
{
    const auto sbegin = s.find_first_not_of(" \t\r\n");
    const auto send = s.find_last_not_of(" \t\r\n");
    if(sbegin != std::string::npos)
        return s.substr(sbegin, send - sbegin + 1);
    return std::string{};
}

void config::load_cmdline()
{
    std::string value = {};
    for(const auto it : map) {
        if(!cmdline::get_value(it.first, value))
            continue;
        it.second->from_string(value);
    }
}

void config::load_file(const vfs::path_t &path)
{
    if(vfs::file_t *file = vfs::open(path, vfs::OPEN_RD)) {
        std::string line = {};

        while(vfs::read_line(file, line)) {
            if(const size_t pos = line.find('='); pos != std::string::npos) {
                const auto it = map.find(trim_whitespace(line.substr(0, pos)));
                if(it == map.cend())
                    continue;
                it->second->from_string(trim_whitespace(line.substr(pos + 1)));
            }
        }
    }
}

void config::save_file(const vfs::path_t &path)
{
    std::ostringstream oss = {};
    for(const auto it : map)
        oss << fmt::format("{} = {}", it.first, it.second->to_string()) << std::endl;
    vfs::write_string(path, oss.str());
}

void config::add(const std::string &name, config::IVariable &vref)
{
    map[name] = &vref;
}
