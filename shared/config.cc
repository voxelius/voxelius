// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <algorithm>
#include <ctype.h> // isspace
#include <inttypes.h> // strtoimax, strtoumax
#include <shared/cmdline.hh>
#include <shared/config.hh>
#include <stdlib.h> // strtod
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

config::Boolean::Boolean(bool default_value)
    : default_value{default_value}, value{default_value}
{

}

void config::Boolean::from_string(const std::string &sval)
{
    if(!sval.compare("true")) {
        value = true;
        return;
    }

    if(!sval.compare("false")) {
        value = false;
        return;
    }

    value = default_value;
}

std::string config::Boolean::to_string() const
{
    return std::string{value ? "true" : "false"};
}

void config::Boolean::set_value(bool value)
{
    this->value = value;
}

bool config::Boolean::get_default_value() const
{
    return default_value;
}

bool config::Boolean::get_value() const
{
    return value;
}

config::String::String(const std::string &default_value)
    : default_value{default_value}, value{default_value}
{

}

void config::String::from_string(const std::string &sval)
{
    value = sval;
}

std::string config::String::to_string() const
{
    return value;
}

void config::String::set_value(const std::string &value)
{
    this->value = value;
}

const std::string &config::String::get_default_value() const
{
    return default_value;
}

const std::string &config::String::get_value() const
{
    return value;
}

config::Number::Number(double default_value)
    : default_value{default_value}, value{default_value}
{

}

void config::Number::from_string(const std::string &sval)
{
    value = strtod(sval.c_str(), nullptr);
}

std::string config::Number::to_string() const
{
    return std::to_string(value);
}

void config::Number::set_value(double value)
{
    this->value = value;
}

double config::Number::get_default_value() const
{
    return default_value;
}

double config::Number::get_value() const
{
    return value;
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
        std::string key = {};
        std::string val = {};

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
        oss << it.first << " = " << it.second->to_string() << std::endl;
    vfs::write_string(path, oss.str());
}

void config::add_variable(const std::string &name, config::IVariable &vref)
{
    map[name] = &vref;
}
