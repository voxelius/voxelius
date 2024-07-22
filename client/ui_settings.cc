// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/camera.hh>
#include <client/event/glfw_key.hh>
#include <client/event/language_set.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/mouse.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <shared/config.hh>

constexpr static ImGuiWindowFlags MENU_FLAGS =
    ImGuiWindowFlags_NoTitleBar         |
    ImGuiWindowFlags_NoResize           |
    ImGuiWindowFlags_NoCollapse         |
    ImGuiWindowFlags_NoMove             |
    ImGuiWindowFlags_NoMove             |
    ImGuiWindowFlags_NoSavedSettings    |
    ImGuiWindowFlags_NoBackground;

static std::string tab_general = {};
static std::string tab_controls = {};
static std::string tab_graphics = {};
static std::string tab_sound = {};

static std::string separator_multiplayer = {};
static std::string separator_keyboard = {};
static std::string separator_mouse = {};
static std::string separator_gamepad = {};
static std::string separator_performance = {};
static std::string separator_ui = {};

static std::string option_language = {};
static std::string option_fov = {};
static std::string option_username = {};
static std::string option_acceleration = {};
static std::string option_raw_input = {};
static std::string option_pixel_size = {};
static std::string option_view_distance = {};
static std::string option_menu_background = {};

static std::string tooltip_fov = {};
static std::string tooltip_raw_input = {};
static std::string tooltip_menu_background = {};

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


static void on_language_set(const LanguageSetEvent &event)
{
    tab_general = lang::resolve("settings.tab.general") + "###Settings_Tab_General";
    tab_controls = lang::resolve("settings.tab.controls") + "###Settings_Tab_Controls";
    tab_graphics = lang::resolve("settings.tab.graphics") + "###Settings_Tab_Graphics";
    tab_sound = lang::resolve("settings.tab.sound") + "###Settings_Tab_Sound";

    separator_multiplayer = lang::resolve("settings.separator.multiplayer") + "###Settings_Separator_Multiplayer";
    separator_keyboard = lang::resolve("settings.separator.keyboard") + "###Settings_Separator_Keyboard";
    separator_mouse = lang::resolve("settings.separator.mouse") + "###Settings_Separator_Mouse";
    separator_gamepad = lang::resolve("settings.separator.gamepad") + "###Settings_Separator_Gamepad";
    separator_performance = lang::resolve("settings.separator.performance") + "###Settings_Separator_Performance";
    separator_ui = lang::resolve("settings.separator.ui") + "###Settings_Separator_UI";

    option_language = lang::resolve("settings.option.language") + "###Settings_Option_Language";
    option_fov = lang::resolve("settings.option.fov") + "###Settings_Option_FOV";
    option_username = lang::resolve("settings.option.username") + "###Settings_Option_Username";
    option_acceleration = lang::resolve("settings.option.acceleration") + "###Settings_Option_Acceleration";
    option_raw_input = lang::resolve("settings.option.raw_input") + "###Settings_Option_RawInput";
    option_pixel_size = lang::resolve("settings.option.pixel_size") + "###Settings_Option_PixelSize";
    option_view_distance = lang::resolve("settings.option.view_distance") + "###Settings_Option_ViewDistance";
    option_menu_background = lang::resolve("settings.option.menu_background") + "###Settings_Option_MenuBackground";

    tooltip_fov = lang::resolve("settings.tooltip.fov");
    tooltip_raw_input = lang::resolve("settings.tooltip.raw_input");
    tooltip_menu_background = lang::resolve("settings.tooltip.menu_background");
}

static void settings_tooltip(const std::string &tooltip)
{
    ImGui::SameLine();
    ImGui::TextDisabled("[ ? ]");

    if(ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 16.0f);
        ImGui::TextUnformatted(tooltip.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
static void layout_general(void)
{
    // FIXME: there may be the case when the manifest
    // is completely messed up and this iterator is invalid
    const auto current_lang = lang::current();

    if(ImGui::BeginCombo(option_language.c_str(), current_lang->display.c_str())) {
        for(auto it = lang::cbegin(); it != lang::cend(); ++it) {
            if(ImGui::Selectable(it->display.c_str(), (it == current_lang))) {
                lang::set(it);
                continue;
            }
        }

        ImGui::EndCombo();
    }

    int fov_value = static_cast<int>(camera::horizontal_fov);
    ImGui::SliderInt(option_fov.c_str(), &fov_value, 54, 110);
    settings_tooltip(tooltip_fov);
    camera::horizontal_fov = fov_value;

    ImGui::SeparatorText(separator_multiplayer.c_str());

    // FIXME: make this readonly when the player
    // is connected to the server - changing usernames
    // dynamically shouldn't be allowed by servers
    ImGui::InputText(option_username.c_str(), &client_game::username, ImGuiInputTextFlags_CharsNoBlank);
}

static void layout_controls()
{
    ImGui::SeparatorText(separator_keyboard.c_str());
    ImGui::NewLine();

    ImGui::Spacing();
    ImGui::SeparatorText(separator_mouse.c_str());

    float sensitivity_value = 100.0f * static_cast<float>(mouse::sensitivity);
    ImGui::SliderFloat(option_acceleration.c_str(), &sensitivity_value, 15.0f, 100.0f, "%.0f");
    mouse::sensitivity = sensitivity_value * 0.01f;

    ImGui::Checkbox(option_raw_input.c_str(), &mouse::raw_input);
    settings_tooltip(tooltip_raw_input);

    ImGui::Spacing();
    ImGui::SeparatorText(separator_gamepad.c_str());
    ImGui::NewLine();
}

static void layout_graphics(void)
{
    ImGui::SeparatorText(separator_performance.c_str());

    int pixel_value = static_cast<int>(client_game::pixel_size);
    ImGui::SliderInt(option_pixel_size.c_str(), &pixel_value, 1, 4);
    client_game::pixel_size = pixel_value;

    int view_value = static_cast<int>(camera::view_distance);
    ImGui::SliderInt(option_view_distance.c_str(), &view_value, 1, 32);
    camera::view_distance = view_value;

    ImGui::Spacing();
    ImGui::SeparatorText(separator_ui.c_str());

    ImGui::Checkbox(option_menu_background.c_str(), &client_game::menu_background);
    settings_tooltip(tooltip_menu_background);
}

static void layout_sound(void)
{
    ImGui::TextUnformatted("There's no sound yet");
}

void ui::settings::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void ui::settings::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(viewport->Size.x * 0.10f, viewport->Size.y * 0.10f);
    const ImVec2 window_size = ImVec2(viewport->Size.x * 0.80f, viewport->Size.y * 0.80f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("##Settings", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * globals::ui_scale, 3.0f * globals::ui_scale));

        if(ImGui::BeginTabBar("##Settings_Tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                // Go back to the main menu
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem(tab_general.c_str())) {
                if(ImGui::BeginChild("###Settings_General_Child"))
                    layout_general();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem(tab_controls.c_str())) {
                if(ImGui::BeginChild("###Settings_Controls_Child"))
                    layout_controls();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem(tab_graphics.c_str())) {
                if(ImGui::BeginChild("###Settings_Graphics_Child"))
                    layout_graphics();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // STOP COPYPASTING CODE
            if(ImGui::BeginTabItem(tab_sound.c_str())) {
                if(ImGui::BeginChild("###Settings_Sound_Child"))
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
