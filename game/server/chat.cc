// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/server/chat.hh>
#include <game/server/globals.hh>
#include <game/server/sessions.hh>
#include <game/shared/protocol.hh>
#include <spdlog/spdlog.h>

static void on_chat_message_packet(const protocol::ChatMessage &packet)
{
    if(packet.type == protocol::ChatMessage::TEXT_MESSAGE) {
        if(packet.peer->data)
            server_chat::broadcast(packet.message, reinterpret_cast<const Session *>(packet.peer->data)->username);
        else server_chat::broadcast(packet.message, packet.sender);
    }
}

void server_chat::init(void)
{
    globals::dispatcher.sink<protocol::ChatMessage>().connect<&on_chat_message_packet>();
}

void server_chat::broadcast(const std::string &message)
{
    server_chat::broadcast(message, std::string("server"));
}

void server_chat::broadcast(const std::string &message, const std::string &sender)
{
    protocol::ChatMessage packet = {};
    packet.type = protocol::ChatMessage::TEXT_MESSAGE;
    packet.message = message;
    packet.sender = sender;
    protocol::send(nullptr, globals::server_host, packet);
    spdlog::info("[{}] {}", sender, message);
}

void server_chat::send(Session *session, const std::string &message)
{
    server_chat::send(session, message, std::string("server"));
}

void server_chat::send(Session *session, const std::string &message, const std::string &sender)
{
    protocol::ChatMessage packet = {};
    packet.type = protocol::ChatMessage::TEXT_MESSAGE;
    packet.message = message;
    packet.sender = sender;
    protocol::send(nullptr, globals::server_host, packet);
}
