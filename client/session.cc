// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/gui_screen.hh>
#include <client/message_box.hh>
#include <client/progress.hh>
#include <client/session.hh>
#include <shared/protocol.hh>
#include <spdlog/spdlog.h>

static void on_login_response(const protocol::LoginResponse &packet)
{
    spdlog::info("session: assigned session_id={}", packet.session_id);
    spdlog::info("session: assigned username: {}", packet.username);
    spdlog::info("session: server ticks at {} TPS", packet.tickrate);
    
    globals::session_id = packet.session_id;
    globals::session_tick_dt = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(cxpr::max<std::uint16_t>(10, packet.tickrate)));
    globals::session_username = packet.username;
    
    progress::set_title("Loading world");    
}

static void on_disconnect(const protocol::Disconnect &packet)
{
    enet_peer_disconnect(globals::session_peer, 0);
    
    spdlog::info("session: disconnected: {}", packet.reason);

    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_username = std::string();

    message_box::reset();
    message_box::set_title("Disconnected");
    message_box::set_title(packet.reason);
    message_box::add_button("Back to Menu", [](void) {
        globals::gui_screen = GUI_MAIN_MENU;
    });
    
    globals::gui_screen = GUI_MESSAGE_BOX;
}

void session::init(void)
{
    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_username = std::string();
    
    globals::dispatcher.sink<protocol::LoginResponse>().connect<&on_login_response>();
    globals::dispatcher.sink<protocol::Disconnect>().connect<&on_disconnect>();
}

void session::deinit(void)
{
    session::disconnect("disconnect.client_shutdown");

    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_username = std::string();
}

void session::connect(const std::string &host, std::uint16_t port)
{
    ENetAddress address = {};
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    
    globals::session_peer = enet_host_connect(globals::client_host, &address, 1, 0);
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_username = std::string();
    
    if(!globals::session_peer) {
        message_box::reset();
        message_box::set_title("Connection failed");
        message_box::set_title("Cannot create ENetPeer");
        message_box::add_button("Back to Menu", [](void) {
            globals::gui_screen = GUI_MAIN_MENU;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;

        return;
    }

    progress::reset();
    progress::set_title("Connecting");
    progress::set_button("Cancel", [](void) {
        enet_peer_disconnect(globals::session_peer, 0);
        enet_host_flush(globals::client_host);

        globals::session_peer = nullptr;
        globals::session_id = UINT16_MAX;
        globals::session_tick_dt = UINT64_MAX;
        globals::session_username = std::string();

        globals::gui_screen = GUI_MAIN_MENU;
    });

    globals::gui_screen = GUI_PROGRESS;
}

void session::disconnect(const std::string &reason)
{
    if(globals::session_peer) {
        protocol::Disconnect packet = {};
        packet.reason = reason;
        protocol::send_packet(globals::session_peer, packet);
        enet_host_flush(globals::client_host);

        globals::session_peer = nullptr;
        globals::session_id = UINT16_MAX;
        globals::session_tick_dt = UINT64_MAX;
        globals::session_username = std::string();
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
    
    protocol::send_packet(globals::session_peer, packet);
    
    progress::set_title("Logging in");
    globals::gui_screen = GUI_PROGRESS;
}

void session::invalidate(void)
{
    if(globals::session_peer) {
        enet_peer_reset(globals::session_peer);
        
        message_box::reset();
        message_box::set_title("Disconnected");
        message_box::set_subtitle("Connection terminated");
        message_box::add_button("Back to Menu", [](void) {
            globals::gui_screen = GUI_MAIN_MENU;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;
    }

    globals::session_peer = nullptr;
    globals::session_id = UINT16_MAX;
    globals::session_tick_dt = UINT64_MAX;
    globals::session_username = std::string();
}
