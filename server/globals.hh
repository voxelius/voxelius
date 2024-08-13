// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_GLOBALS_HH
#define SERVER_GLOBALS_HH
#include <shared/globals.hh>

class Config;

namespace globals
{
extern Config server_config;

extern bool is_running;
extern unsigned int tickrate;
extern std::uint64_t tickrate_dt;
} // namespace globals

#endif /* SERVER_GLOBALS_HH */
