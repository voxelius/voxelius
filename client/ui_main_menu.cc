// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/debug_session.hh>
#include <client/event/glfw_key.hh>
#include <client/event/lang_set.hh>
#include <client/globals.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>
#include <shared/cmake.hh>

constexpr static const ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

static std::string id_debug_session = {};
static std::string id_server_list = {};
static std::string id_settings = {};
static std::string id_quit = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::registry.valid(globals::player)) {
        if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
            switch(globals::ui_screen) {
                case ui::SCREEN_NONE:
                    globals::ui_screen = ui::SCREEN_MAIN_MENU;
                    break;
                case ui::SCREEN_MAIN_MENU:
                    globals::ui_screen = ui::SCREEN_NONE;
                    break;
            }
        }
    }
}

static void on_lang_set(const LangSetEvent &event)
{
    id_debug_session = lang::find("ui.main_menu.debug_session") + std::string{"###Menu_DebugSession"};
    id_server_list = lang::find("ui.main_menu.server_list") + std::string{"###Menu_ServerList"};
    id_settings = lang::find("ui.main_menu.settings") + std::string{"###Menu_Settings"};
    id_quit = lang::find("ui.main_menu.quit") + std::string{"###Menu_Quit"};
}

void ui::main_menu::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LangSetEvent>().connect<&on_lang_set>();
}

void ui::main_menu::layout()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2{0.0f, viewport->Size.y * 0.10f};
    const ImVec2 window_size = ImVec2{viewport->Size.x, viewport->Size.y * 0.80f};

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###MainMenu", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0f, 2.0f * globals::ui_scale});

        ImGui::PushFont(globals::font_menu_title);
        constexpr static const char *title_str = "Voxelius";
        const float title_width = ImGui::CalcTextSize(title_str).x;
        const float title_xpos = 0.5f * (window_size.x - title_width);
        ImGui::SetCursorPosX(title_xpos);
        ImGui::TextUnformatted(title_str);
        ImGui::PopFont();

        constexpr static const char *subtitle_str = VOXELIUS_SEMVER;
        const float subtitle_width = ImGui::CalcTextSize(subtitle_str).x;
        const float subtitle_xpos = 0.5f * (window_size.x - subtitle_width);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.0f * globals::ui_scale);
        ImGui::SetCursorPosX(subtitle_xpos);
        ImGui::TextUnformatted(subtitle_str);

        ImGui::Dummy(ImVec2{0.0f, 10.0f * globals::ui_scale});

        const float button_width = 192.0f * globals::ui_scale;
        const float button_xpos = 0.5f * (window_size.x - button_width);

        ImGui::PushFont(globals::font_menu_button);
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button(id_debug_session.c_str(), ImVec2{button_width, 0.0f}))
            debug_session::run();
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button(id_server_list.c_str(), ImVec2{button_width, 0.0f}))
            globals::ui_screen = ui::SCREEN_SERVER_LIST;
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button(id_settings.c_str(), ImVec2{button_width, 0.0f}))
            globals::ui_screen = ui::SCREEN_SETTINGS;
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button(id_quit.c_str(), ImVec2{button_width, 0.0f}))
            glfwSetWindowShouldClose(globals::window, true);
        ImGui::PopFont();

        ImGui::PopStyleVar();
        ImGui::SameLine();
    }

    ImGui::End();
}
