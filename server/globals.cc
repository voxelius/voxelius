// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <server/globals.hh>
#include <shared/config.hh>

Config globals::server_config = {};

ENetHost *globals::server_host = {};

bool globals::is_running = false;
unsigned int globals::tickrate = 30U;
std::uint64_t globals::tickrate_dt = 0;
