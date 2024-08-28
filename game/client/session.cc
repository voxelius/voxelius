// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/game.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/message_box.hh>
#include <game/client/progress.hh>
#include <game/client/session.hh>
#include <game/shared/event/chunk_update.hh>
#include <game/shared/event/voxel_set.hh>
#include <game/shared/coord.hh>
#include <game/shared/protocol.hh>
#include <game/shared/world.hh>
#include <spdlog/spdlog.h>

static void on_login_response_packet(const protocol::LoginResponse &packet)
{
    spdlog::info("session: assigned session_id={}", packet.session_id);
    spdlog::info("session: assigned username: {}", packet.username);
    spdlog::info("session: server ticks at {} TPS", packet.tickrate);
    
    globals::session_id = packet.session_id;
    globals::session_tick_dt = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(cxpr::max<std::uint16_t>(10, packet.tickrate)));
    globals::session_send_time = 0;
    globals::session_username = packet.username;
    
    progress::set_title("connecting.loading_world");    
}

static void on_disconnect_packet(const protocol::Disconnect &packet)
{
    enet_peer_disconnect(globals::session_peer, 0);

    spdlog::info("session: disconnected: {}", packet.reason);

    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_send_time = UINT64_MAX;
    globals::session_username = std::string();

    globals::player = entt::null;
    globals::registry.clear();

    message_box::reset();
    message_box::set_title("disconnected.disconnected");
    message_box::set_subtitle(packet.reason);
    message_box::add_button("disconnected.back", [](void) {
        globals::gui_screen = GUI_PLAY_MENU;
    });

    globals::gui_screen = GUI_MESSAGE_BOX;
}

static void on_set_voxel_packet(const protocol::SetVoxel &packet)
{
    const auto cpos = VoxelCoord::to_chunk(packet.coord);
    const auto lpos = VoxelCoord::to_local(packet.coord);
    const auto index = LocalCoord::to_index(lpos);

    if(Chunk *chunk = world::find(cpos)) {
        if(chunk->voxels[index] != packet.voxel) {
            chunk->voxels[index] = packet.voxel;
            
            ChunkUpdateEvent event = {};
            event.coord = cpos;
            event.chunk = chunk;
            
            // Send a generic ChunkUpdate event to shake
            // up the mesher; directly calling world::set_voxel
            // here would result in a networked feedback loop
            // caused by event handler below tripping
            globals::dispatcher.trigger(event);
        }
    }
}

// NOTE: [session] is a good place for this since [receive]
// handles entity data sent by the server and [session] handles
// everything else network related that is not player movement
static void on_voxel_set(const VoxelSetEvent &event)
{
    if(globals::session_peer) {
        // Propagate changes to the server
        protocol::send_set_voxel(globals::session_peer, nullptr, event.vpos, event.voxel);
    }
}

void session::init(void)
{
    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_send_time = UINT64_MAX;
    globals::session_username = std::string();
    
    globals::dispatcher.sink<protocol::LoginResponse>().connect<&on_login_response_packet>();
    globals::dispatcher.sink<protocol::Disconnect>().connect<&on_disconnect_packet>();
    globals::dispatcher.sink<protocol::SetVoxel>().connect<&on_set_voxel_packet>();

    globals::dispatcher.sink<VoxelSetEvent>().connect<&on_voxel_set>();
}

void session::deinit(void)
{
    session::disconnect("protocol.client_shutdown");
    globals::session_send_time = UINT64_MAX;
}

void session::connect(const std::string &host, std::uint16_t port)
{
    ENetAddress address = {};
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    
    globals::session_peer = enet_host_connect(globals::client_host, &address, 1, 0);
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_send_time = UINT64_MAX;
    globals::session_username = std::string();
    
    if(!globals::session_peer) {
        message_box::reset();
        message_box::set_title("disconnected.disconnected");
        message_box::set_subtitle("enet.peer_connection_failed");
        message_box::add_button("disconnected.back", [](void) {
            globals::gui_screen = GUI_PLAY_MENU;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;

        return;
    }

    progress::reset();
    progress::set_title("connecting.connecting");
    progress::set_button("connecting.cancel_button", [](void) {
        enet_peer_disconnect(globals::session_peer, 0);

        globals::session_peer = nullptr;
        globals::session_id = UINT16_MAX;
        globals::session_tick_dt = UINT64_MAX;
        globals::session_send_time = UINT64_MAX;
        globals::session_username = std::string();

        globals::gui_screen = GUI_MAIN_MENU;
    });

    globals::gui_screen = GUI_PROGRESS;
}

void session::disconnect(const std::string &reason)
{
    if(globals::session_peer) {
        protocol::send_disconnect(globals::session_peer, nullptr, reason);

        enet_host_flush(globals::client_host);

        globals::session_peer = nullptr;
        globals::session_id = UINT16_MAX;
        globals::session_tick_dt = UINT64_MAX;
        globals::session_username = std::string();

        globals::player = entt::null;
        globals::registry.clear();
    }
}

void session::send_login_request(void)
{
    protocol::LoginRequest packet = {};
    packet.version = protocol::VERSION;
    packet.password_hash = UINT64_MAX; // FIXME
    packet.vdef_checksum = UINT64_MAX; // FIXME
    packet.player_uid = client_game::player_uid;
    packet.username = client_game::username;
    protocol::send(globals::session_peer, nullptr, packet);
    
    progress::set_title("connecting.logging_in");
    globals::gui_screen = GUI_PROGRESS;
}

void session::invalidate(void)
{
    if(globals::session_peer) {
        enet_peer_reset(globals::session_peer);
        
        message_box::reset();
        message_box::set_title("disconnected.disconnected");
        message_box::set_subtitle("enet.peer_connection_timeout");
        message_box::add_button("disconnected.back", [](void) {
            globals::gui_screen = GUI_PLAY_MENU;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;
    }

    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_send_time = UINT64_MAX;
    globals::session_username = std::string();
}
