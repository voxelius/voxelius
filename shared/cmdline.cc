// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <shared/cmdline.hh>
#include <shared/strtools.hh>

// Valid options always start with
// at least one OPT_CHAR and never end with one
constexpr static char OPT_CHAR = '-';

static emhash8::HashMap<std::string, std::string> options = {};

static bool is_argv_opt(const std::string &str)
{
    if(str.find_last_of(OPT_CHAR) >= (str.size() - 1))
        return false;
    return str[0] == OPT_CHAR;
}

static std::string get_argv_opt(const std::string &str)
{
    std::size_t i;
    for(i = 0; str[i] == OPT_CHAR; ++i);
    return std::string(str.cbegin() + i, str.cend());
}

void cmdline::append(int argc, char **argv)
{
    for(int i = 1; i < argc; ++i) {
        if(!is_argv_opt(argv[i]))
            continue;

        const auto opt = get_argv_opt(argv[i]);
        const auto next_i = i + 1;

        if(strtools::is_empty_or_whitespace(opt))
            continue;

        if(next_i < argc && !is_argv_opt(argv[next_i])) {
            options.insert_or_assign(opt, argv[next_i]);
            i = next_i;
            continue;
        }

        options.insert_or_assign(opt, std::string());
        continue;
    }
}

void cmdline::append(const std::string &opt)
{
    options.insert_or_assign(opt, std::string());
}

void cmdline::append(const std::string &opt, const std::string &arg)
{
    options.insert_or_assign(opt, std::string(arg));
}

bool cmdline::get_value(const std::string &opt, std::string &arg)
{
    if(const auto it = options.find(opt); it != options.cend()) {
        arg.assign(it->second);
        return true;
    }

    arg.clear();
    return false;
}

bool cmdline::contains(const std::string &opt)
{
    if(options.find(opt) != options.cend())
        return true;
    return false;
}
