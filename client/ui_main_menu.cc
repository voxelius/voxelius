// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/debug_session.hh>
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>
#include <shared/cmake.hh>

constexpr static const ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

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

void ui::main_menu::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
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

        ImGui::PushFont(globals::font_default);
        constexpr static const char *subtitle_str = VOXELIUS_SEMVER;
        const float subtitle_width = ImGui::CalcTextSize(subtitle_str).x;
        const float subtitle_xpos = 0.5f * (window_size.x - subtitle_width);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.0f * globals::ui_scale);
        ImGui::SetCursorPosX(subtitle_xpos);
        ImGui::TextUnformatted(subtitle_str);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2{0.0f, 10.0f * globals::ui_scale});

        const float button_width = 192.0f * globals::ui_scale;
        const float button_xpos = 0.5f * (window_size.x - button_width);

        ImGui::PushFont(globals::font_menu_button);
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button("DEBUG SESSION###MainMenu_DebugSession", ImVec2{button_width, 0.0f}))
            debug_session::run();
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button("Join a Server###MainMenu_JoinGame", ImVec2{button_width, 0.0f}))
            globals::ui_screen = ui::SCREEN_SERVER_LIST;
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button("Settings###MainMenu_Settings", ImVec2{button_width, 0.0f}))
            globals::ui_screen = ui::SCREEN_SETTINGS;
        ImGui::Spacing();
        ImGui::SetCursorPosX(button_xpos);
        if(ImGui::Button("Quit Game###MainMenu_Quit", ImVec2{button_width, 0.0f}))
            glfwSetWindowShouldClose(globals::window, true);
        ImGui::PopFont();

        ImGui::PopStyleVar();
        ImGui::SameLine();
    }

    ImGui::End();
}
