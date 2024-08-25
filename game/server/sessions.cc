// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/server/globals.hh>
#include <game/server/send.hh>
#include <game/server/sessions.hh>
#include <game/shared/protocol.hh>
#include <mathlib/constexpr.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

// This is not a good idea
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>

unsigned int sessions::max_players = 16U;
unsigned int sessions::num_players = 0U;

static std::unordered_map<std::uint64_t, Session *> sessions_map = {};
static std::vector<Session> sessions_vector = {};

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

static void on_login_request(const protocol::LoginRequest &packet)
{
    
    if(packet.version > protocol::VERSION) {
        protocol::Disconnect response = {};
        response.reason = "Outdated server";
        protocol::send_packet(packet.peer, response);
        return;
    }
    
    if(packet.version < protocol::VERSION) {
        protocol::Disconnect response = {};
        response.reason = "Outdated client";
        protocol::send_packet(packet.peer, response);
        return;
    }
    
    if(Session *session = sessions::create(packet.peer, packet.player_uid, packet.username)) {
        protocol::LoginResponse response = {};
        response.session_id = session->session_id;
        response.tickrate = globals::tickrate;
        response.username = session->username;
        protocol::send_packet(packet.peer, response);

        spdlog::info("sessions: {} [{}] logged in with session_id={}", session->username, session->player_uid, session->session_id);

        session->player = globals::registry.create();
        globals::registry.emplace<HeadComponent>(session->player, HeadComponent());
        globals::registry.emplace<PlayerComponent>(session->player, PlayerComponent());
        globals::registry.emplace<TransformComponent>(session->player, TransformComponent());
        globals::registry.emplace<VelocityComponent>(session->player, VelocityComponent());

        // FIXME: this is not a good idea
        globals::registry.each([session](const entt::entity entity) {
            server_send::chunk(session, entity);
            server_send::head(session, entity);
            server_send::transform(session, entity);
            server_send::velocity(session, entity);
        });

        protocol::SpawnPlayer result = {};
        result.entity = session->player;
        protocol::send_packet(session->peer, result);

        return;
    }
    
    protocol::Disconnect response = {};
    response.reason = "Too much players";
    protocol::send_packet(packet.peer, response);
}

static void on_disconnect(const protocol::Disconnect &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        spdlog::info("sessions: {} disconnected: {}", session->username, packet.reason);
        if(globals::registry.valid(session->player))
            globals::registry.destroy(session->player);
        sessions::destroy(session);
    }
}

void sessions::init(void)
{
    Config::add(globals::server_config, "sessions.max_players", sessions::max_players);

    globals::dispatcher.sink<protocol::LoginRequest>().connect<&on_login_request>();
    globals::dispatcher.sink<protocol::Disconnect>().connect<&on_disconnect>();
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

            peer->data = &sessions_vector[i];

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
    return reinterpret_cast<Session *>(peer->data);
}

void sessions::destroy(Session *session)
{
    if(session) {
        if(session->peer) {
            // Make sure we don't leave a mark
            session->peer->data = nullptr;
        }
        
        sessions_map.erase(session->player_uid);

        session->session_id = UINT16_MAX;
        session->player_uid = UINT64_MAX;
        session->username = std::string();
        session->player = entt::null;
        session->peer = nullptr;

        sessions::num_players -= 1U;
    }
}
