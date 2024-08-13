// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_GAMESTATE_HH
#define SHARED_GAMESTATE_HH
#include <cstdint>

enum class GameState : std::uint16_t {
    Disconnected    = 0x0000,
    Connecting      = 0x0001,
    LoggingIn       = 0x0002,
    LoadingWorld    = 0x0003,
    Playing         = 0x0004,
};

#endif /* SHARED_GAMESTATE_HH */
