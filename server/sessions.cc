// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/config.hh>
#include <server/globals.hh>
#include <server/session.hh>
#include <unordered_map>
#include <vector>

static std::unordered_map<std::uint64_t, Session *> userid_cache = {};
static std::vector<Session> sessions = {};

unsigned int Session::max_players = 16U;
unsigned int Session::num_players = 0U;

void Session::init(void)
{
    Config::add(globals::server_config, "max_players", Session::max_players);
}

void Session::init_late(void)
{
    Session::max_players = cxpr::clamp<unsigned int>(Session::max_players, 1U, UINT16_MAX);
    Session::num_players = 0U;

    sessions.resize(Session::max_players, Session());

    for(unsigned int i = 0U; i < Session::max_players; ++i) {
        sessions[i].state = SessionState::Offline;
        sessions[i].session_id = UINT16_MAX;
        sessions[i].player_userid = UINT64_MAX;
        sessions[i].timeout_epoch = UINT64_MAX;
        sessions[i].player_entity = entt::null;
        sessions[i].peer = nullptr;
    }
}

void Session::deinit(void)
{
    sessions.clear();
}

Session *Session::create(ENetPeer *peer)
{
    for(unsigned int i = 0U; i < Session::max_players; ++i) {
        if(sessions[i].state == SessionState::Offline) {
            sessions[i].state = SessionState::Limbo;
            sessions[i].session_id = i;
            sessions[i].player_userid = UINT64_MAX;
            sessions[i].timeout_epoch = UINT64_MAX;
            sessions[i].player_entity = entt::null;
            sessions[i].peer = peer;
            
            peer->data = &sessions[i];
            
            
        }
    }
    
    
}

Session *Session::find(std::uint16_t session_id)
{
}

Session *Session::find(std::uint64_t player_userid)
{
}

void Session::destroy(Session *session)
{
}
