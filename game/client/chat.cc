// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <deque>
#include <common/config.hh>
#include <common/strtools.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/chat.hh>
#include <game/client/game.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/settings.hh>
#include <game/shared/protocol.hh>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

struct GuiChatMessage final {
    std::uint64_t spawn {};
    std::string text {};
    ImVec4 color {};
};

static int key_chat = GLFW_KEY_ENTER;
static std::deque<GuiChatMessage> history = {};
static std::string chat_input = {};
static bool needs_focus = false;

static void on_chat_message_packet(const protocol::ChatMessage &packet)
{
    if(packet.type == protocol::ChatMessage::PLAYER_MSG) {
        GuiChatMessage message = {};
        message.spawn = globals::curtime;
        message.text = fmt::format("<{}> {}", packet.sender, packet.message);
        message.color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        history.push_back(message);
        return;
    }
    
    if(packet.type == protocol::ChatMessage::PLAYER_JOIN) {
        GuiChatMessage message = {};
        message.spawn = globals::curtime;
        message.text = fmt::format("{} joined the game", packet.sender);
        message.color = ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget);
        history.push_back(message);
        return;
    }
    
    if(packet.type == protocol::ChatMessage::PLAYER_LEAVE) {
        GuiChatMessage message = {};
        message.spawn = globals::curtime;
        message.text = fmt::format("{} left the game ({})", packet.sender, packet.message);
        message.color = ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget);
        history.push_back(message);
        return;
    }
    
    if(packet.type == protocol::ChatMessage::SERVER_MSG) {
        GuiChatMessage message = {};
        message.spawn = globals::curtime;
        message.text = fmt::format("[{}] {}", packet.sender, packet.message);
        message.color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        history.push_back(message);
        return;
    }
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        if((event.key == GLFW_KEY_ENTER) && (globals::gui_screen == GUI_CHAT)) {
            if(!strtools::is_empty_or_whitespace(chat_input)) {
                if(globals::session_peer) {
                    protocol::ChatMessage packet = {};
                    packet.type = protocol::ChatMessage::PLAYER_MSG;
                    packet.sender = globals::session_username;
                    packet.message = chat_input;
                    protocol::send(globals::session_peer, nullptr, packet);
                }
                else {
                    GuiChatMessage message = {};
                    message.text = fmt::format("<{}> {}", client_game::username, chat_input);
                    message.color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                    history.push_back(message);
                }
            }

            globals::gui_screen = GUI_SCREEN_NONE;

            chat_input.clear();

            return;
        }
        
        if((event.key == GLFW_KEY_ESCAPE) && (globals::gui_screen == GUI_CHAT)) {
            globals::gui_screen = GUI_SCREEN_NONE;
            return;
        }
        
        if((event.key == key_chat) && !globals::gui_screen) {
            globals::gui_screen = GUI_CHAT;
            needs_focus = true;
            return;
        }
    }
}

void client_chat::init(void)
{
    Config::add(globals::client_config, "chat.key", key_chat);

    settings::add_key_binding(2, settings::KEYBOARD_MISC, "key.chat", key_chat);

    globals::dispatcher.sink<protocol::ChatMessage>().connect<&on_chat_message_packet>();
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void client_chat::init_late(void)
{

}

void client_chat::update(void)
{
}

void client_chat::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(0.0f, 0.0f);
    const ImVec2 window_size = ImVec2(0.75f * viewport->Size.x, viewport->Size.y);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    ImGui::PushFont(globals::font_chat);

    if(ImGui::Begin("###chat", nullptr, WINDOW_FLAGS)) {
        const ImVec2 &cursor = ImGui::GetCursorScreenPos();
        const ImVec2 &padding = ImGui::GetStyle().FramePadding;
        const ImVec2 &spacing = ImGui::GetStyle().ItemSpacing;
        const ImFont *font = ImGui::GetFont();

        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        // The text input widget occupies the bottom part
        // of the chat window, we need to reserve some space for it
        float ypos = window_size.y - font->FontSize - 2.0f * padding.y - 2.0f * spacing.y;

        if(globals::gui_screen == GUI_CHAT) {
            if(needs_focus) {
                ImGui::SetKeyboardFocusHere();
                needs_focus = false;
            }

            ImGui::SetNextItemWidth(window_size.x - 2.0f * padding.x);
            ImGui::SetCursorScreenPos(ImVec2(padding.x, ypos));
            ImGui::InputText("###chat.input", &chat_input);
        }

        if((globals::gui_screen == GUI_SCREEN_NONE) || (globals::gui_screen == GUI_CHAT)) {
            for(auto it = history.crbegin(); it < history.crend(); ++it) {
                const ImVec2 text_size = ImGui::CalcTextSize(it->text.c_str(), &it->text.cend()[0], false, window_size.x);
                const ImVec2 rect_size = ImVec2(window_size.x, text_size.y + 2.0f * padding.y);

                const ImVec2 rect_pos = ImVec2(padding.x, ypos - text_size.y - 2.0f * padding.y);
                const ImVec2 rect_end = ImVec2(rect_pos.x + rect_size.x, rect_pos.y + rect_size.y);
                const ImVec2 text_pos = ImVec2(rect_pos.x + padding.x, rect_pos.y + padding.y);
                
                const float fadeout_seconds = 10.0f;
                const float fadeout = std::exp(-1.0f * std::pow(1.0e-6 * static_cast<float>(globals::curtime - it->spawn) / fadeout_seconds, 10.0f));
                const float rect_alpha = ((globals::gui_screen == GUI_CHAT) ? (0.75f) : (0.50f * fadeout));
                const float text_alpha = ((globals::gui_screen == GUI_CHAT) ? (1.00f) : (1.00f * fadeout));
                const float chat_height = ((globals::gui_screen == GUI_CHAT) ? (0.50f) : (0.75f));

                if((ypos - rect_size.y) < (window_size.y * chat_height)) {
                    // Cull out messages that are not
                    // supposed to be visible anymore
                    break;
                }
                
                const ImU32 rect_col = ImGui::GetColorU32(ImGuiCol_FrameBg, rect_alpha);
                const ImU32 text_col = ImGui::GetColorU32(ImVec4(it->color.x, it->color.y, it->color.z, it->color.w * text_alpha));

                draw_list->AddRectFilled(rect_pos, rect_end, rect_col);
                draw_list->AddText(font, font->FontSize, text_pos, text_col, it->text.c_str(), &it->text.cend()[0], window_size.x);

                ypos -= rect_size.y;
            }
        }
    }

    ImGui::End();
    ImGui::PopFont();
}

void client_chat::clear(void)
{
    history.clear();
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
