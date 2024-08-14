// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <server/globals.hh>
#include <server/send.hh>
#include <server/sessions.hh>
#include <shared/protocol.hh>
#include <shared/world.hh>

void server_send::chunk(Session *session, entt::entity entity)
{
    if(session && globals::registry.valid(entity)) {
        if(const ChunkComponent *component = globals::registry.try_get<ChunkComponent>(entity)) {
            protocol::ChunkVoxels packet = {};
            packet.entity = entity;
            packet.chunk = component->coord;
            packet.voxels = component->chunk->voxels;

            protocol::send_packet(session->peer, packet);
        }
    }
}

void server_send::head(Session *session, entt::entity entity)
{
    if(session && globals::registry.valid(entity)) {
        if(const HeadComponent *component = globals::registry.try_get<HeadComponent>(entity)) {
            protocol::EntityHead packet = {};
            packet.entity = entity;
            packet.angles = component->angles;

            protocol::send_packet(session->peer, packet);
        }
    }
}

void server_send::transform(Session *session, entt::entity entity)
{
    if(session && globals::registry.valid(entity)) {
        if(const TransformComponent *component = globals::registry.try_get<TransformComponent>(entity)) {
            protocol::EntityTransform packet = {};
            packet.entity = entity;
            packet.angles = component->angles;
            packet.coord = component->position;

            protocol::send_packet(session->peer, packet);
        }
    }
}

void server_send::velocity(Session *session, entt::entity entity)
{
    if(session && globals::registry.valid(entity)) {
        if(const VelocityComponent *component = globals::registry.try_get<VelocityComponent>(entity)) {
            protocol::EntityVelocity packet = {};
            packet.entity = entity;
            packet.angular = component->angular;
            packet.linear = component->linear;

            protocol::send_packet(session->peer, packet);
        }
    }
}
