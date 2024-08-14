// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <shared/math/constexpr.hh>
#include <shared/config.hh>
#include <shared/protocol.hh>
#include <server/globals.hh>
#include <server/sessions.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

unsigned int sessions::max_players = 16U;
unsigned int sessions::num_players = 0U;

static std::unordered_map<std::uint64_t, Session *> sessions_map = {};
static std::vector<Session> sessions_vector = {};

static void on_login_request(const protocol::LoginRequest &packet)
{
    if(packet.version != protocol::VERSION) {
        protocol::Disconnect disconnect = {};
        disconnect.reason = "Protocol version mismatch";

        protocol::send_packet(packet.peer, disconnect);
        enet_host_flush(globals::host);
        enet_peer_disconnect(packet.peer, 0);

        return;
    }

    Session *session = sessions::create(packet.peer, packet.player_uid, packet.username);

    if(!session) {
        protocol::Disconnect disconnect = {};
        disconnect.reason = "Too much players";

        protocol::send_packet(packet.peer, disconnect);
        enet_host_flush(globals::host);
        enet_peer_disconnect(packet.peer, 0);

        return;
    }

    protocol::LoginResponse response = {};
    response.session_id = session->session_id;
    response.tickrate = globals::tickrate;
    response.username = session->username;

    protocol::send_packet(packet.peer, response);

    spdlog::info("sent response to [{}] ({})", session->username, packet.username);
}

static std::string make_unique_username(const std::string &username)
{
    for(const Session &session : sessions_vector) {
        if(session.peer) {
            if(session.username.compare(username))
                continue;
            return make_unique_username(username + std::string("(1)"));
        }
    }

    return username;
}

void sessions::init(void)
{
    Config::add(globals::server_config, "sessions.max_players", sessions::max_players);

    globals::dispatcher.sink<protocol::LoginRequest>().connect<&on_login_request>();
}

void sessions::init_late(void)
{
    sessions::max_players = cxpr::clamp<unsigned int>(sessions::max_players, 1U, UINT16_MAX);
    sessions::num_players = 0U;

    sessions_vector.resize(sessions::max_players, Session());

    for(unsigned int i = 0U; i < sessions::max_players; ++i) {
        sessions_vector[i].session_id = UINT16_MAX;
        sessions_vector[i].player_uid = UINT64_MAX;
        sessions_vector[i].username = std::string();
        sessions_vector[i].player = entt::null;
        sessions_vector[i].peer = nullptr;
    }
}

void sessions::deinit(void)
{
    sessions_map.clear();
    sessions_vector.clear();
}

Session *sessions::create(ENetPeer *peer, std::uint64_t player_uid, const std::string &username)
{
    for(unsigned int i = 0U; i < sessions::max_players; ++i) {
        if(!sessions_vector[i].peer) {
            sessions_vector[i].session_id = i;
            sessions_vector[i].player_uid = player_uid;
            sessions_vector[i].username = make_unique_username(username);
            sessions_vector[i].player = entt::null;
            sessions_vector[i].peer = peer;        

            sessions_map[player_uid] = &sessions_vector[i];

            sessions::num_players += 1U;

            return &sessions_vector[i];
        }
    }

    return nullptr;
}

Session *sessions::find(std::uint16_t session_id)
{
    if(session_id < sessions_vector.size()) {
        if(!sessions_vector[session_id].peer)
            return nullptr;
        return &sessions_vector[session_id];
    }

    return nullptr;
}

Session *sessions::find(std::uint64_t player_uid)
{
    const auto it = sessions_map.find(player_uid);
    if(it != sessions_map.cend())
        return it->second;
    return nullptr;
}

Session *sessions::find(ENetPeer *peer)
{
    return reinterpret_cast<Session *>(peer);
}

void sessions::destroy(Session *session)
{
    if(session) {
        session->peer->data = nullptr;
        
        sessions_map.erase(session->player_uid);

        session->session_id = UINT16_MAX;
        session->player_uid = UINT64_MAX;
        session->username = std::string();
        session->player = entt::null;
        session->peer = nullptr;

        sessions::num_players -= 1U;
    }
}
