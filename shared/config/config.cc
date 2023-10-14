// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/cmdline.hh>
#include <shared/config/config.hh>
#include <spdlog/fmt/fmt.h>
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
