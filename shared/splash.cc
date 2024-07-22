// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <random>
#include <shared/util/physfs.hh>
#include <shared/splash.hh>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

static std::mt19937_64 twister = {};
static std::vector<std::string> splashes = {};

void splash::init(void)
{
    const std::string path = std::string("/misc/splashes.txt");

    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::warn("splash: {}: {}", path, util::physfs_error());
        splashes.push_back("Splashes file didn't load...");
        splashes.push_back("YOU SHALL NOT SPLASH!!1!");
    }
    else {
        std::string line = {};
        std::istringstream stream = std::istringstream(source);
        while(std::getline(stream, line)) splashes.push_back(line);
    }

    twister.seed(std::random_device()());
}

const std::string &splash::get(void)
{
    return splashes.at(twister() % splashes.size());
}
