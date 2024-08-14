// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <client/globals.hh>
#include <client/gui_screen.hh>
#include <client/message_box.hh>
#include <client/progress.hh>
#include <client/session.hh>
#include <shared/protocol.hh>
#include <spdlog/spdlog.h>

ENetPeer *session::peer = nullptr;
std::uint16_t session::session_id = UINT16_MAX;
std::uint64_t session::tick_time = UINT64_MAX;
std::string session::username = std::string();

static void on_login_response(const protocol::LoginResponse &packet)
{
    spdlog::info("session: assigned session ID {}", packet.session_id);
    spdlog::info("session: {} -> {}", session::username, packet.username);
    spdlog::info("session: server ticks at {} TPS", packet.tickrate);

    session::session_id = packet.session_id;
    session::tick_time = static_cast<std::uint64_t>(1000000.0f / static_cast<float>(cxpr::max<std::uint16_t>(10, packet.tickrate)));
    session::username = packet.username;

    progress::set_title("Loading world");
}

static void on_disconnect(const protocol::Disconnect &packet)
{
    spdlog::info("session: {}", packet.reason);

    session::peer = nullptr;
    session::session_id = UINT16_MAX;
    session::tick_time = UINT64_MAX;
    session::username = std::string();

    message_box::reset();
    message_box::set_title("Disconnected");
    message_box::set_subtitle(packet.reason);
    message_box::add_button("Back to Menu", [](void) {
        globals::gui_screen = GUI_MAIN_MENU;
    });

    globals::gui_screen = GUI_MESSAGE_BOX;
}

void session::init(void)
{
    session::peer = nullptr;
    session::session_id = UINT16_MAX;
    session::tick_time = UINT64_MAX;
    session::username = std::string();

    globals::dispatcher.sink<protocol::LoginResponse>().connect<&on_login_response>();
    globals::dispatcher.sink<protocol::Disconnect>().connect<&on_disconnect>();
}

void session::connect(const std::string &host, std::uint16_t port, const std::string &username)
{
    ENetAddress address = {};
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    session::peer = enet_host_connect(globals::host, &address, 1, 0);

    if(!session::peer) {
        message_box::reset();
        message_box::set_title("Connection failed");
        message_box::set_subtitle("Host connection failed");
        message_box::add_button("Back to Menu", [](void) {
            // FIXME: make it the server list
            globals::gui_screen = GUI_MAIN_MENU;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;

        return;
    }

    session::username = username;

    progress::reset();
    progress::set_title("Connecting");

    globals::gui_screen = GUI_PROGRESS;
}

void session::disconnect(const std::string &reason)
{
    if(session::peer) {
        protocol::Disconnect packet = {};
        packet.reason = reason;

        protocol::send_packet(session::peer, packet);
        enet_host_flush(globals::host);
        enet_peer_reset(session::peer);

        session::peer = nullptr;
        session::session_id = UINT16_MAX;
        session::tick_time = UINT64_MAX;
        session::username = std::string();
    }
}
