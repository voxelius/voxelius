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

static void on_chunk_voxels(const protocol::ChunkVoxels &packet)
{
    if(!globals::session_peer) return;

    if(!globals::registry.valid(packet.entity)) {
        entt::entity created = globals::registry.create(packet.entity);

        if(created != packet.entity) {
            globals::registry.destroy(created);
            session::disconnect("chunk entity mismatch");
            spdlog::critical("receive: chunk entity mismatch");
            return;
        }
    }

    Chunk *chunk = world::assign(packet.chunk, packet.entity);
    chunk->voxels = packet.voxels;
}

static void on_entity_head(const protocol::EntityHead &packet)
{
    if(!globals::session_peer) return;

    if(!globals::registry.valid(packet.entity))
        static_cast<void>(globals::registry.create(packet.entity));
    auto &component = globals::registry.get_or_emplace<HeadComponent>(packet.entity);
    component.angles = packet.angles;
}

static void on_entity_transform(const protocol::EntityTransform &packet)
{
    if(!globals::session_peer) return;

    if(!globals::registry.valid(packet.entity))
        static_cast<void>(globals::registry.create(packet.entity));
    auto &component = globals::registry.get_or_emplace<TransformComponent>(packet.entity);
    component.angles = packet.angles;
    component.position = packet.coord;
}

static void on_entity_velocity(const protocol::EntityVelocity &packet)
{
    if(!globals::session_peer) return;

    if(!globals::registry.valid(packet.entity))
        static_cast<void>(globals::registry.create(packet.entity));
    auto &component = globals::registry.get_or_emplace<VelocityComponent>(packet.entity);
    component.angular = packet.angular;
    component.linear = packet.linear;
}

static void on_spawn_player(const protocol::SpawnPlayer &packet)
{
    if(!globals::session_peer) return;

    if(!globals::registry.valid(packet.entity))
        static_cast<void>(globals::registry.create(packet.entity));
    globals::registry.get_or_emplace<PlayerComponent>(packet.entity);
    globals::player = packet.entity;
    globals::gui_screen = GUI_SCREEN_NONE;
}

void client_receive::init(void)
{
    globals::dispatcher.sink<protocol::ChunkVoxels>().connect<&on_chunk_voxels>();
    globals::dispatcher.sink<protocol::EntityHead>().connect<&on_entity_head>();
    globals::dispatcher.sink<protocol::EntityTransform>().connect<&on_entity_transform>();
    globals::dispatcher.sink<protocol::EntityVelocity>().connect<&on_entity_velocity>();
    globals::dispatcher.sink<protocol::SpawnPlayer>().connect<&on_spawn_player>();
}