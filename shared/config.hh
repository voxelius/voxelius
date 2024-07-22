// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CONFIG_HH
#define SHARED_CONFIG_HH
#include <string>
#include <unordered_map>

using ConfigVarType = unsigned short;
constexpr static ConfigVarType CONFIG_INT           = 0x0000;
constexpr static ConfigVarType CONFIG_BOOLEAN       = 0x0001;
constexpr static ConfigVarType CONFIG_FLOAT         = 0x0002;
constexpr static ConfigVarType CONFIG_STD_STRING    = 0x0003;
constexpr static ConfigVarType CONFIG_UNSIGNED_INT  = 0x0004;

struct ConfigVariable final {
    ConfigVarType type {};
    void *value_ptr {};
};

class Config final {
private:
    std::unordered_map<std::string, ConfigVariable> vmap {};

public:
    static void add(Config &config, const std::string &name, int &vref);
    static void add(Config &config, const std::string &name, bool &vref);
    static void add(Config &config, const std::string &name, float &vref);
    static void add(Config &config, const std::string &name, std::string &vref);
    static void add(Config &config, const std::string &name, unsigned int &vref);

public:
    static void clear(Config &config);
    static bool load(Config &config, const std::string &path);
    static bool save(const Config &config, const std::string &path);
};

#endif /* SHARED_CONFIG_HH */
