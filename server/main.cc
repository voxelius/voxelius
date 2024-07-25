// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/cmake.hh>
#include <server/main.hh>
#include <spdlog/spdlog.h>

void server::main(void)
{
    spdlog::info("server: game version: {}", GAME_VERSION_STRING);
    std::terminate();
}
