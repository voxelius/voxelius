// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/protocol.hh>
#include <server/globals.hh>
#include <server/receive.hh>
#include <server/sessions.hh>

static void on_entity_transform(const protocol::EntityTransform &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &transform = globals::registry.get_or_emplace<TransformComponent>(session->player);
            transform.angles = packet.angles;
            transform.position = packet.coord;
        }
    }
}

static void on_entity_velocity(const protocol::EntityVelocity &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &velocity = globals::registry.get_or_emplace<VelocityComponent>(session->player);
            velocity.angular = packet.angular;
            velocity.linear = packet.linear;
        }
    }
}

static void on_entity_head(const protocol::EntityHead &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &transform = globals::registry.get_or_emplace<HeadComponent>(session->player);
            transform.angles = packet.angles;
        }
    }
}

void server_recieve::init(void)
{
    globals::dispatcher.sink<protocol::EntityTransform>().connect<&on_entity_transform>();
    globals::dispatcher.sink<protocol::EntityVelocity>().connect<&on_entity_velocity>();
    globals::dispatcher.sink<protocol::EntityHead>().connect<&on_entity_head>();
}
