// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_SESSION_HH
#define SERVER_SESSION_HH
#include <cstdint>
#include <enet/enet.h>
#include <entt/entity/entity.hpp>
#include <vector>

constexpr static unsigned short SESSION_INVAL   = 0x0000;
constexpr static unsigned short SESSION_STATUS  = 0x0001;
constexpr static unsigned short SESSION_LOGIN   = 0x0002;

struct Session final {
public:
    unsigned short type {};
    std::uint64_t timeout {};
    std::uint16_t session_id {};
    ENetPeer *peer {};

public:
    std::uint64_t player_userid {};
    std::string player_username {};
    entt::entity player_entity {};
};





#endif /* SERVER_SESSION_HH */
