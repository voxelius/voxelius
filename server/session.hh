// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_SESSION_HH
#define SERVER_SESSION_HH
#include <cstdint>
#include <enet/enet.h>
#include <entt/entity/entity.hpp>
#include <vector>

enum class SessionState {
    Disconnected    = 0x0000, // Session is free to be reused
    LoadingWorld    = 0x0001, // The client is receiving world data
    Playing         = 0x0002, // The client is playing the videogame 
};

struct Session final {
    ENetPeer *peer {nullptr};
    std::uint16_t session_id {};
    std::uint64_t player_uid {};
    entt::entity player_entity {};
    SessionState state {};

public:
    static unsigned int max_players;

public:
    static void init(void);
    static void init_late(void);
    static void deinit(void);

public:
    static Session *create(ENetPeer *peer, std::uint64_t player_uid);
    static Session *find(std::uint16_t session_id);
    static Session *find(std::uint64_t player_uid);
    static void destroy(Session *session);
};

#endif /* SERVER_SESSION_HH */
