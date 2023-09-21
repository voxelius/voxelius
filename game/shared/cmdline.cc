// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <algorithm>
#include <ctype.h>
#include <game/shared/cmdline.hh>
#include <unordered_map>
#include <unordered_set>

static inline bool is_valid_key(const std::string &argv)
{
    if(argv.find_last_of('-') >= argv.size() - 1)
        return false;
    return argv[0] == '-';
}

static inline const std::string get_key(const std::string &argv)
{
    size_t i;
    for(i = 0; argv[i] == '-'; i++);
    return std::string(argv.cbegin() + i, argv.cend());
}

// UNDONE: move this to strtools
static inline bool is_empty_or_whitespace(const std::string &s)
{
    if(s.empty())
        return true;
    return std::all_of(s.cbegin(), s.cend(), &isspace);
}

static std::unordered_map<std::string, std::string> argv_map = {};
static std::unordered_set<std::string> argv_set {};

void cmdline::append(int argc, char **argv)
{
    for(int i = 1; i < argc; ++i) {
        if(is_valid_key(argv[i])) {
            const std::string key = get_key(argv[i]);

            if(!is_empty_or_whitespace(key)) {
                if(((i + 1) < argc) && !is_valid_key(argv[i + 1])) {
                    argv_map[key] = argv[++i];
                    argv_set.erase(key);
                    continue;
                }

                argv_map.erase(key);
                argv_set.insert(key);
            }
        }
    }
}

void cmdline::append(const std::string &key)
{
    argv_map.erase(key);
    argv_set.insert(key);
}

void cmdline::append(const std::string &key, const std::string &value)
{
    argv_map[key] = value;
    argv_set.erase(key);
}

bool cmdline::contains(const std::string &key)
{
    return argv_set.count(key) || argv_map.count(key);
}

bool cmdline::has_value(const std::string &key)
{
    return argv_map.count(key);
}

bool cmdline::get_value(const std::string &key, std::string &value)
{
    const auto it = argv_map.find(key);
    const auto jt = argv_set.find(key);

    if(it != argv_map.cend()) {
        value.assign(it->second);
        return true;
    }
    else if(jt != argv_set.cend()) {
        value.clear();
        return true;
    }
    else {
        value.clear();
        return false;
    }
}
