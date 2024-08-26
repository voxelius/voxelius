// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <entt/entity/entity.hpp>
#include <string>

struct Session final {
    std::uint16_t session_id {};
    std::uint64_t player_uid {};
    std::string username {};
    entt::entity player {};
    ENetPeer *peer {};
};

namespace sessions
{
extern unsigned int max_players;
extern unsigned int num_players;
} // namespace sessions

namespace sessions
{
void init(void);
void init_late(void);
void deinit(void);
} // namespace sessions

namespace sessions
{
Session *create(ENetPeer *peer, std::uint64_t player_uid, const std::string &username);
Session *find(std::uint16_t session_id);
Session *find(std::uint64_t player_uid);
Session *find(ENetPeer *peer);
void destroy(Session *session);
} // namespace sessions

namespace sessions
{
void send_disconnect(ENetPeer *peer, const std::string &reason);
void send_chat_message(ENetPeer *peer, const std::string &message);
} // namespace sessions

namespace sessions
{
void send_chunk_voxels(ENetPeer *peer, entt::entity entity);
void send_entity_head(ENetPeer *peer, entt::entity entity);
void send_entity_transform(ENetPeer *peer, entt::entity entity);
void send_entity_velocity(ENetPeer *peer, entt::entity entity);
} // namespace sessions

namespace sessions
{
void send_spawn_player(ENetPeer *peer, entt::entity entity);
} // namespace sessions
