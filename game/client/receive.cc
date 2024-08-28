// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/receive.hh>
#include <game/client/session.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/protocol.hh>
#include <game/shared/world.hh>
#include <spdlog/spdlog.h>

static bool make_entity(entt::entity entity)
{
    if(!globals::registry.valid(entity)) {
        entt::entity created = globals::registry.create(entity);

        if(created != entity) {
            session::disconnect("protocol.chunk_entity_mismatch");
            spdlog::critical("receive: chunk entity mismatch");
            return false;
        }
    }

    return true;
}

static void on_chunk_voxels_packet(const protocol::ChunkVoxels &packet)
{
    if(globals::session_peer) {
        if(!globals::registry.valid(packet.entity)) {
            entt::entity created = globals::registry.create(packet.entity);

            if(created != packet.entity) {
                globals::registry.destroy(created);
                session::disconnect("protocol.chunk_entity_mismatch");
                spdlog::critical("receive: chunk entity mismatch");
                return;
            }
        }

        Chunk *chunk = world::assign(packet.chunk, packet.entity);
        chunk->voxels = packet.voxels;
    }
}

static void on_entity_head_packet(const protocol::EntityHead &packet)
{
    if(globals::session_peer) {
        if(!make_entity(packet.entity))
            return;
        auto &component = globals::registry.get_or_emplace<HeadComponent>(packet.entity);
        component.angles = packet.angles;
    }
}

static void on_entity_transform_packet(const protocol::EntityTransform &packet)
{
    if(globals::session_peer) {
        if(!globals::registry.valid(packet.entity))
            static_cast<void>(globals::registry.create(packet.entity));
        auto &component = globals::registry.get_or_emplace<TransformComponent>(packet.entity);
        component.angles = packet.angles;
        component.position = packet.coord;
    }
}

static void on_entity_velocity_packet(const protocol::EntityVelocity &packet)
{
    if(globals::session_peer) {
        if(!globals::registry.valid(packet.entity))
            static_cast<void>(globals::registry.create(packet.entity));
        auto &component = globals::registry.get_or_emplace<VelocityComponent>(packet.entity);
        component.angular = packet.angular;
        component.linear = packet.linear;
    }
}

static void on_spawn_player_packet(const protocol::SpawnPlayer &packet)
{
    if(globals::session_peer) {
        if(!globals::registry.valid(packet.entity))
            static_cast<void>(globals::registry.create(packet.entity));
        globals::registry.get_or_emplace<PlayerComponent>(packet.entity);
        globals::player = packet.entity;
        globals::gui_screen = GUI_SCREEN_NONE;
    }
}

static void on_remove_entity_packet(const protocol::RemoveEntity &packet)
{
    if(globals::registry.valid(packet.entity)) {
        if(packet.entity == globals::player)
            globals::player = entt::null;
        globals::registry.destroy(packet.entity);
    }
}

void client_receive::init(void)
{
    globals::dispatcher.sink<protocol::ChunkVoxels>().connect<&on_chunk_voxels_packet>();
    globals::dispatcher.sink<protocol::EntityHead>().connect<&on_entity_head_packet>();
    globals::dispatcher.sink<protocol::EntityTransform>().connect<&on_entity_transform_packet>();
    globals::dispatcher.sink<protocol::EntityVelocity>().connect<&on_entity_velocity_packet>();
    globals::dispatcher.sink<protocol::SpawnPlayer>().connect<&on_spawn_player_packet>();
    globals::dispatcher.sink<protocol::RemoveEntity>().connect<&on_remove_entity_packet>();
}
