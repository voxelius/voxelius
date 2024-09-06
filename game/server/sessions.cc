// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/server/globals.hh>
#include <game/server/sessions.hh>
#include <game/shared/chunk.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/event/chunk_create.hh>
#include <game/shared/event/voxel_set.hh>
#include <game/shared/protocol.hh>
#include <mathlib/constexpr.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

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

static void on_login_request_packet(const protocol::LoginRequest &packet)
{
    if(packet.version > protocol::VERSION) {
        protocol::send_disconnect(packet.peer, nullptr, "protocol.outdated_server");
        return;
    }

    if(packet.version < protocol::VERSION) {
        protocol::send_disconnect(packet.peer, nullptr, "protocol.outdated_client");
        return;
    }
    
    if(Session *session = sessions::create(packet.peer, packet.player_uid, packet.username)) {
        protocol::LoginResponse response = {};
        response.session_id = session->session_id;
        response.tickrate = globals::tickrate;
        response.username = session->username;
        protocol::send(packet.peer, nullptr, response);

        spdlog::info("sessions: {} [{}] logged in with session_id={}", session->username, session->player_uid, session->session_id);

        // FIXME: this is not a good idea
        for(auto entity : globals::registry.view<entt::entity>()) {
            protocol::send_chunk_voxels(session->peer, nullptr, entity);
            protocol::send_entity_head(session->peer, nullptr, entity);
            protocol::send_entity_transform(session->peer, nullptr, entity);
            protocol::send_entity_velocity(session->peer, nullptr, entity);
            protocol::send_entity_player(session->peer, nullptr, entity);
        }

        session->player = globals::registry.create();
        globals::registry.emplace<HeadComponent>(session->player, HeadComponent());
        globals::registry.emplace<PlayerComponent>(session->player, PlayerComponent());
        globals::registry.emplace<TransformComponent>(session->player, TransformComponent());
        globals::registry.emplace<VelocityComponent>(session->player, VelocityComponent());

        // The player entity is to be spawned in the world the last;
        // We don't want to interact with the still not-loaded world!
        protocol::send_entity_head(nullptr, globals::server_host, session->player);
        protocol::send_entity_transform(nullptr, globals::server_host, session->player);
        protocol::send_entity_velocity(nullptr, globals::server_host, session->player);
        protocol::send_entity_player(nullptr, globals::server_host, session->player);

        // SpawnPlayer serves a different purpose compared to EntityPlayer
        // The latter is used to construct entities (as in "attach a component")
        // whilst the SpawnPlayer packet is used to notify client-side that the
        // entity identifier in the packet is to be treated as the local player entity
        protocol::send_spawn_player(session->peer, nullptr, session->player);

        protocol::ChatMessage message = {};
        message.type = protocol::ChatMessage::PLAYER_JOIN;
        message.sender = session->username;
        message.message = std::string();
        protocol::send(nullptr, globals::server_host, message);

        return;
    }

    protocol::send_disconnect(packet.peer, nullptr, "protocol.max_players");
}

static void on_disconnect_packet(const protocol::Disconnect &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        protocol::ChatMessage message = {};
        message.type = protocol::ChatMessage::PLAYER_LEAVE;
        message.sender = session->username;
        message.message = packet.reason;
        protocol::send(session->peer, globals::server_host, message);

        spdlog::info("{} disconnected ({})", session->username, packet.reason);

        sessions::destroy(session);
    }
}

// NOTE: [sessions] is a good place for this since [receive]
// handles entity data sent by players and [sessions] handles
// everything else network related that is not player movement
static void on_chunk_create(const ChunkCreateEvent &event)
{
    protocol::ChunkVoxels packet = {};
    packet.entity = event.chunk->entity;
    packet.chunk = event.coord;
    packet.voxels = event.chunk->voxels;
    protocol::send(nullptr, globals::server_host, packet);
}

static void on_voxel_set(const VoxelSetEvent &event)
{
    protocol::send_set_voxel(nullptr, globals::server_host, event.vpos, event.voxel);
}

static void on_destroy_entity(const entt::registry &registry, entt::entity entity)
{
    protocol::RemoveEntity packet = {};
    packet.entity = entity;
    protocol::send(nullptr, globals::server_host, packet);
}

void sessions::init(void)
{
    Config::add(globals::server_config, "sessions.max_players", sessions::max_players);

    globals::dispatcher.sink<protocol::LoginRequest>().connect<&on_login_request_packet>();
    globals::dispatcher.sink<protocol::Disconnect>().connect<&on_disconnect_packet>();

    globals::dispatcher.sink<ChunkCreateEvent>().connect<&on_chunk_create>();
    globals::dispatcher.sink<VoxelSetEvent>().connect<&on_voxel_set>();

    globals::registry.on_destroy<entt::entity>().connect<&on_destroy_entity>();
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
        
        globals::registry.destroy(session->player);

        sessions_map.erase(session->player_uid);

        session->session_id = UINT16_MAX;
        session->player_uid = UINT64_MAX;
        session->username = std::string();
        session->player = entt::null;
        session->peer = nullptr;

        sessions::num_players -= 1U;
    }
}
