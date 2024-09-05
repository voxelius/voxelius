// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/server/chat.hh>
#include <game/server/globals.hh>
#include <game/shared/protocol.hh>
#include <spdlog/spdlog.h>

static void on_chat_message_packet(const protocol::ChatMessage &packet)
{
    if(packet.type == protocol::ChatMessage::PLAYER_MSG) {
        spdlog::info("<{}> {}", packet.sender, packet.message);
        protocol::send(nullptr, globals::server_host, packet);
    }
}

void server_chat::init(void)
{
    globals::dispatcher.sink<protocol::ChatMessage>().connect<&on_chat_message_packet>();
}

void server_chat::send(const std::string &message)
{
    protocol::ChatMessage packet = {};
    packet.type = protocol::ChatMessage::SERVER_MSG;
    packet.sender = std::string("server");
    packet.message = message;
    protocol::send(nullptr, globals::server_host, packet);
}
