// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/event/glfw_key.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/player_look.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <shared/config/boolean.hh>
#include <shared/config/number.hh>
#include <shared/config/string.hh>

constexpr static const ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        switch(globals::ui_screen) {
            case ui::SCREEN_SETTINGS:
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
                break;
        }
    }
}

static void layout_general()
{
    int fov_value = camera::fov.value;
    ImGui::SliderInt("FOV", &fov_value, 54, 110);
    camera::fov.value = fov_value;

    ImGui::Spacing();

    if(ImGui::CollapsingHeader("Multiplayer")) {
        // FIXME: make this readonly when the player
        // is connected to the server - changing usernames
        // dynamically shouldn't be allowed by servers
        ImGui::InputText("Username", &client_game::username.value, ImGuiInputTextFlags_CharsNoBlank);
    }
}

static void layout_controls()
{
    if(ImGui::CollapsingHeader("Keyboard")) {
        ImGui::TextUnformatted("Nothing to see here");
        ImGui::TextUnformatted("Key bindings are not implemented yet");
        ImGui::Spacing();
    }

    if(ImGui::CollapsingHeader("Mouse")) {
        float sensitivity_value = player_look::sensitivity.value;
        ImGui::SliderFloat("Acceleration", &sensitivity_value, 0.25, 0.50, "%.02f");
        player_look::sensitivity.value = sensitivity_value;

        ImGui::Checkbox("Raw input", &player_look::raw_input.value);

        ImGui::Spacing();
    }

    if(ImGui::CollapsingHeader("Gamepad")) {
        ImGui::TextUnformatted("Nothing to see here");
        ImGui::TextUnformatted("Gamepad support is not implemented yet");
        ImGui::Spacing();
    }
}

static void layout_graphics()
{
    if(ImGui::CollapsingHeader("Performance")) {
        int pixel_value = client_game::pixel_size.value;
        ImGui::SliderInt("Pixel size", &pixel_value, 1, 4);
        client_game::pixel_size.value = pixel_value;

        int view_value = camera::view_distance.value;
        ImGui::SliderInt("View distance", &view_value, 1, 32);
        camera::view_distance.value = view_value;

        ImGui::Spacing();
    }

    if(ImGui::CollapsingHeader("UI")) {
        ImGui::Checkbox("Menu background", &client_game::menu_background.value);
        ImGui::Spacing();
    }
}

static void layout_sound()
{
    ImGui::TextUnformatted("There's no sound yet");
}

void ui::settings::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void ui::settings::layout()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2{viewport->Size.x * 0.10f, viewport->Size.y * 0.10f};
    const ImVec2 window_size = ImVec2{viewport->Size.x * 0.80f, viewport->Size.y * 0.80f};

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###Settings", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{2.0f * globals::ui_scale, 2.0f * globals::ui_scale});

        if(ImGui::BeginTabBar("###Settings_Tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                // Go back to the main menu
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem("General")) {
                if(ImGui::BeginChild("###Settings_General"))
                    layout_general();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem("Controls")) {
                if(ImGui::BeginChild("###Settings_Controls"))
                    layout_controls();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem("Graphics")) {
                if(ImGui::BeginChild("###Settings_Graphics"))
                    layout_graphics();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem("Sound")) {
                if(ImGui::BeginChild("###Settings_Sound"))
                    layout_sound();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}
