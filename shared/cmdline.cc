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
#include <shared/strtools.hh>
#include <unordered_map>

// Valid options always start with
// at least one OPT_CHAR and never end with one
constexpr static char OPT_CHAR = '-';

static bool is_argv_opt(const std::string &str)
{
    if(str.find_last_of(OPT_CHAR) >= (str.size() - 1))
        return false;
    return str[0] == OPT_CHAR;
}

static std::string get_argv_opt(const std::string &str)
{
    size_t i;
    for(i = 0; str[i] == OPT_CHAR; ++i);
    return std::string{str.cbegin() + i, str.cend()};
}

static std::unordered_map<std::string, std::string> options = {};

void cmdline::add(int argc, char **argv)
{
    for(int i = 1; i < argc; ++i) {
        if(is_argv_opt(argv[i])) {
            const auto opt = get_argv_opt(argv[i]);
            const auto next_i = i + 1;

            if(!strtools::is_empty_or_whitespace(opt)) {
                if(next_i < argc && !is_argv_opt(argv[next_i])) {
                    options.insert_or_assign(opt, argv[next_i]);
                    i = next_i;
                    continue;
                }

                options.insert_or_assign(opt, std::string{});
                continue;
            }
        }

    }
}

void cmdline::add(const std::string &opt)
{
    options.insert_or_assign(opt, std::string{});
}

void cmdline::add(const std::string &opt, const std::string &arg)
{
    options.insert_or_assign(opt, arg);
}

bool cmdline::get(const std::string &opt, std::string &arg)
{
    if(const auto it = options.find(opt); it != options.cend()) {
        arg.assign(it->second);
        return true;
    }

    arg.clear();
    return false;
}

bool cmdline::has(const std::string &opt)
{
    if(options.find(opt) != options.cend())
        return true;
    return false;
}
