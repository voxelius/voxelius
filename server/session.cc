// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/config.hh>
#include <server/globals.hh>
#include <server/session.hh>
#include <unordered_map>
#include <vector>

static std::unordered_map<std::uint64_t, Session *> sessions_map = {};
static std::vector<Session> sessions_vector = {};

unsigned int Session::max_players = 16U;

void Session::init(void)
{
    Config::add(globals::server_config, "network.max_players", Session::max_players);
}

void Session::init_late(void)
{
    Session::max_players = cxpr::clamp<unsigned int>(Session::max_players, 1U, UINT16_MAX);
    sessions_vector.resize(Session::max_players);
    sessions_map.clear();

    for(std::size_t i = 0; i < sessions_vector.size(); ++i) {
        sessions_vector[i].peer = nullptr;
        sessions_vector[i].session_id = UINT16_MAX;
        sessions_vector[i].player_uid = UINT64_MAX;
        sessions_vector[i].player_entity = entt::null;
        sessions_vector[i].state = SessionState::Disconnected;
    }
}

void Session::deinit(void)
{
    sessions_vector.clear();
    sessions_map.clear();
}

Session *Session::create(ENetPeer *peer, std::uint64_t player_uid)
{
    for(std::size_t i = 0; i < sessions_vector.size(); ++i) {
        if(sessions_vector[i].state == SessionState::Disconnected) {
            sessions_vector[i].peer = peer;
            sessions_vector[i].session_id = static_cast<std::uint16_t>(i);
            sessions_vector[i].player_uid = player_uid;
            sessions_vector[i].player_entity = entt::null;
            sessions_vector[i].state = SessionState::LoadingWorld;

            sessions_map[player_uid] = &sessions_vector[i];

            peer->data = &sessions_vector[i];

            return &sessions_vector[i];
        }
    }

    return nullptr;
}

Session *Session::find(std::uint16_t session_id)
{
    if(session_id < sessions_vector.size()) {
        if(sessions_vector[session_id].state == SessionState::Disconnected)
            return nullptr;
        return &sessions_vector[session_id];
    }

    return nullptr;
}

Session *Session::find(std::uint64_t player_uid)
{
    const auto it = sessions_map.find(player_uid);
    if(it != sessions_map.cend())
        return it->second;
    return nullptr;
}

void Session::destroy(Session *session)
{
    if(session) {
        sessions_map.erase(session->player_uid);

        session->peer->data = nullptr;

        session->peer = nullptr;
        session->session_id = UINT16_MAX;
        session->player_uid = UINT64_MAX;
        session->player_entity = entt::null;
        session->state = SessionState::Disconnected;
    }
}
