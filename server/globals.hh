// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_GLOBALS_HH
#define SERVER_GLOBALS_HH
#include <enet/enet.h>
#include <shared/globals.hh>

class Config;

namespace globals
{
extern Config server_config;
extern ENetHost *host;
} // namespace globals

#endif /* SERVER_GLOBALS_HH */
