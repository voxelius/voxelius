// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <random>
#include <shared/util/physfs.hh>
#include <shared/titlemsg.hh>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

static std::mt19937_64 twister = {};
static std::vector<std::string> titles = {};

void titlemsg::init(void)
{
    const std::string path = std::string("misc/titles.txt");

    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::warn("splash: {}: {}", path, util::physfs_error());
        titles.push_back("titlemsg::get returned this");
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

const std::string &titlemsg::get(void)
{
    return titles.at(twister() % titles.size());
}
