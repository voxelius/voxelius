// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/fstools.hh>
#include <game/shared/splash.hh>
#include <random>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

static std::mt19937_64 twister = {};
static std::vector<std::string> titles = {};

void splash::init(const std::string &path)
{
    std::string source = {};

    if(!fstools::read_string(path, source)) {
        spdlog::warn("splash: {}: {}", path, fstools::error());
        titles.push_back("splash::get returned this");
    }
    else {
        std::string line = {};
        std::istringstream stream = std::istringstream(source);
        while(std::getline(stream, line)) {
            titles.push_back(line);
        }
    }

    twister.seed(std::random_device()());
}

const std::string &splash::get(void)
{
    auto distrib = std::uniform_int_distribution<std::size_t>(0, titles.size() - 1);
    return titles.at(distrib(twister));
}
