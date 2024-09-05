// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <config/cmake.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/language_set.hh>
#include <game/client/debug_session.hh>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/main_menu.hh>
#include <game/client/session.hh>
#include <imgui.h>

constexpr static ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

static std::string str_play = {};
static std::string str_resume = {};
static std::string str_settings = {};
static std::string str_leave = {};
static std::string str_quit = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::registry.valid(globals::player)) {
            if(globals::gui_screen == GUI_SCREEN_NONE) {
                globals::gui_screen = GUI_MAIN_MENU;
                return;
            }

            if(globals::gui_screen == GUI_MAIN_MENU) {
                globals::gui_screen = GUI_SCREEN_NONE;
                return;
            }
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    str_play = language::resolve_ui("main_menu.play");
    str_resume = language::resolve_ui("main_menu.resume");
    str_settings = language::resolve("main_menu.settings");
    str_leave = language::resolve("main_menu.leave");
    str_quit = language::resolve("main_menu.quit");
}

void main_menu::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void main_menu::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(0.0f, viewport->Size.y * 0.10f);
    const ImVec2 window_size = ImVec2(viewport->Size.x, viewport->Size.y * 0.80f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###main_menu", nullptr, WINDOW_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f * globals::gui_scale));

        ImGui::PushFont(globals::font_title);
        constexpr static const char *title_str = "Voxelius";
        const float title_width = ImGui::CalcTextSize(title_str).x;
        const float title_xpos = 0.5f * (window_size.x - title_width);
        ImGui::SetCursorPosX(title_xpos);
        ImGui::TextUnformatted(title_str);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 10.0f * globals::gui_scale));

        const float button_width = 240.0f * globals::gui_scale;
        const float button_xpos = 0.5f * (window_size.x - button_width);

        if(globals::registry.valid(globals::player)) {
            ImGui::SetCursorPosX(button_xpos);
            if(ImGui::Button(str_resume.c_str(), ImVec2(button_width, 0.0f)))
                globals::gui_screen = GUI_SCREEN_NONE;
            ImGui::Spacing();
        }
        else {
            ImGui::SetCursorPosX(button_xpos);
            if(ImGui::Button(str_play.c_str(), ImVec2(button_width, 0.0f)))
                globals::gui_screen = GUI_PLAY_MENU;
            ImGui::Spacing();
        }

        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button(str_settings.c_str(), ImVec2(button_width, 0.0f)))
            globals::gui_screen = GUI_SETTINGS;
        ImGui::Spacing();

        if(globals::registry.valid(globals::player)) {
            ImGui::SetCursorPosX(button_xpos);
            if(ImGui::Button(str_leave.c_str(), ImVec2(button_width, 0.0f))) {
                session::disconnect("protocol.client_disconnect");
                globals::gui_screen = GUI_PLAY_MENU;
            }

            ImGui::Spacing();
        }
        else {
            ImGui::SetCursorPosX(button_xpos);
            if(ImGui::Button(str_quit.c_str(), ImVec2(button_width, 0.0f)))
                glfwSetWindowShouldClose(globals::window, true);
            ImGui::Spacing();            
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}
