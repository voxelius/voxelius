// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/server/globals.hh>
#include <game/server/receive.hh>
#include <game/server/sessions.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/coord.hh>
#include <game/shared/protocol.hh>
#include <game/shared/world.hh>

static void on_entity_transform_packet(const protocol::EntityTransform &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &transform = globals::registry.get_or_emplace<TransformComponent>(session->player);
            transform.angles = packet.angles;
            transform.position = packet.coord;
        }
    }
}

static void on_entity_velocity_packet(const protocol::EntityVelocity &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &velocity = globals::registry.get_or_emplace<VelocityComponent>(session->player);
            velocity.angular = packet.angular;
            velocity.linear = packet.linear;
        }
    }
}

static void on_entity_head_packet(const protocol::EntityHead &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player) && (packet.entity == entt::null)) {
            auto &transform = globals::registry.get_or_emplace<HeadComponent>(session->player);
            transform.angles = packet.angles;
        }
    }
}

static void on_set_voxel_packet(const protocol::SetVoxel &packet)
{
    if(!world::set_voxel(packet.voxel, packet.coord)) {
        const auto cpos = VoxelCoord::to_chunk(packet.coord);
        const auto lpos = VoxelCoord::to_local(packet.coord);
        const auto index = LocalCoord::to_index(lpos);

        Chunk *chunk = world::assign(cpos, globals::registry.create());
        chunk->voxels[index] = packet.voxel;
        
        // Broadcast the newly created chunk to peers
        sessions::send_chunk_voxels(nullptr, chunk->entity);
    }
}

void server_recieve::init(void)
{
    globals::dispatcher.sink<protocol::EntityTransform>().connect<&on_entity_transform_packet>();
    globals::dispatcher.sink<protocol::EntityVelocity>().connect<&on_entity_velocity_packet>();
    globals::dispatcher.sink<protocol::EntityHead>().connect<&on_entity_head_packet>();
    globals::dispatcher.sink<protocol::SetVoxel>().connect<&on_set_voxel_packet>();
}
