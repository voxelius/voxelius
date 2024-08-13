// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_STATE_HH
#define CLIENT_STATE_HH

enum class ClientState {
    Disconnected    = 0x0000,
    Connecting      = 0x0001,
    Login           = 0x0002,
    LoadingWorld    = 0x0003,
    Playing         = 0x0004,
};

#endif /* CLIENT_STATE_HH */
