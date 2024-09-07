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
#include <game/shared/protocol.hh>
#include <game/shared/world.hh>

static void on_entity_transform_packet(const protocol::EntityTransform &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player)) {
            auto &transform = globals::registry.get_or_emplace<TransformComponent>(session->player);
            transform.angles = packet.angles;
            transform.position = packet.coord;

            // Propagate changes to the rest of the world
            // except the peer that has sent the packet in the first place
            // UNDONE: pass nullptr instead of packet.peer when we want to correct the client
            protocol::send_entity_transform(packet.peer, globals::server_host, session->player);
        }
    }
}

static void on_entity_velocity_packet(const protocol::EntityVelocity &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player)) {
            auto &velocity = globals::registry.get_or_emplace<VelocityComponent>(session->player);
            velocity.angular = packet.angular;
            velocity.linear = packet.linear;

            // Propagate changes to the rest of the world
            // except the peer that has sent the packet in the first place
            // UNDONE: pass nullptr instead of packet.peer when we want to correct the client
            protocol::send_entity_velocity(packet.peer, globals::server_host, session->player);
        }
    }
}

static void on_entity_head_packet(const protocol::EntityHead &packet)
{
    if(Session *session = sessions::find(packet.peer)) {
        if(globals::registry.valid(session->player)) {
            auto &transform = globals::registry.get_or_emplace<HeadComponent>(session->player);
            transform.angles = packet.angles;

            // Propagate changes to the rest of the world
            // except the peer that has sent the packet in the first place
            // UNDONE: pass nullptr instead of packet.peer when we want to correct the client
            protocol::send_entity_head(packet.peer, globals::server_host, session->player);
        }
    }
}

static void on_set_voxel_packet(const protocol::SetVoxel &packet)
{
    if(!world::set_voxel(packet.voxel, packet.coord)) {
        const auto cpos = VoxelCoord::to_chunk(packet.coord);
        const auto lpos = VoxelCoord::to_local(packet.coord);
        const auto index = LocalCoord::to_index(lpos);

        Chunk *chunk = Chunk::create(ChunkType::Inhabited);
        chunk->entity = globals::registry.create();
        chunk->voxels[index] = packet.voxel;

        world::emplace_or_replace(cpos, chunk);
        
        // Broadcast the newly created chunk to peers
        protocol::send_chunk_voxels(nullptr, globals::server_host, chunk->entity);
    }
}

void server_recieve::init(void)
{
    globals::dispatcher.sink<protocol::EntityTransform>().connect<&on_entity_transform_packet>();
    globals::dispatcher.sink<protocol::EntityVelocity>().connect<&on_entity_velocity_packet>();
    globals::dispatcher.sink<protocol::EntityHead>().connect<&on_entity_head_packet>();
    globals::dispatcher.sink<protocol::SetVoxel>().connect<&on_set_voxel_packet>();
}
