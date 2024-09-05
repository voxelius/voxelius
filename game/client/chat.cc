// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <deque>
#include <common/config.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/chat.hh>
#include <game/client/globals.hh>
#include <game/client/settings.hh>
#include <game/shared/protocol.hh>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

struct Message final {
    std::string text {};
    ImU32 color {};
};

static int key_chat = GLFW_KEY_ENTER;
static unsigned int history_size = 64U;
static std::deque<Message> history = {};

static void on_chat_message_packet(const protocol::ChatMessage &packet)
{
    Message message = {};

    switch(packet.type) {
        case protocol::ChatMessage::PLAYER_MSG:
            spdlog::info("<{}> {}", packet.sender, packet.message);
            message.text = fmt::format("<{}> {}", packet.sender, packet.message);
            message.color = ImGui::GetColorU32(ImGuiCol_Text);
            history.push_back(message);
            break;
        case protocol::ChatMessage::PLAYER_JOIN:
            spdlog::info("{} joined the game", packet.sender);
            message.text = fmt::format("{} joined the game", packet.sender);
            message.color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            history.push_back(message);
            break;
        case protocol::ChatMessage::PLAYER_LEAVE:
            spdlog::info("{} left the game ({})", packet.sender, packet.message);
            message.text = fmt::format("{} left the game ({})", packet.sender, packet.message);
            message.color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            history.push_back(message);
            break;
        case protocol::ChatMessage::SERVER_MSG:
            spdlog::warn("[{}] {}", packet.sender, packet.message);
            message.text = fmt::format("[{}] {}", packet.sender, packet.message);
            message.color = ImGui::GetColorU32(ImGuiCol_Text);
            history.push_back(message);
            break;
    }
}

void client_chat::init(void)
{
    Config::add(globals::client_config, "chat.key", key_chat);
    Config::add(globals::client_config, "chat.history_size", history_size);

    settings::add_key_binding(2, settings::KEYBOARD_MISC, "key.chat", key_chat);

    globals::dispatcher.sink<protocol::ChatMessage>().connect<&on_chat_message_packet>();
}

void client_chat::init_late(void)
{

}

void client_chat::update(void)
{
    history_size = cxpr::clamp(history_size, 4U, 2048U);

    while(history.size() > history_size) {
        history.pop_front();
    }
}

void client_chat::layout(void)
{

}

void client_chat::send(const std::string &message)
{
    if(globals::session_peer) {
        protocol::ChatMessage packet = {};
        packet.type = protocol::ChatMessage::PLAYER_MSG;
        packet.sender = globals::session_username;
        packet.message = message;
        protocol::send(globals::session_peer, nullptr, packet);
    }
}
