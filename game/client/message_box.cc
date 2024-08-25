// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/language.hh>
#include <game/client/message_box.hh>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <vector>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

struct Button final {
    std::string str_title {};
    MessageBoxAction action {};
};

static std::string str_title = {};
static std::string str_subtitle = {};
static std::vector<Button> buttons = {};

void message_box::init(void)
{
    str_title = std::string();
    str_subtitle = std::string();
    buttons.clear();
}

void message_box::layout(void)
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(0.0f, viewport->Size.y * 0.30f);
    const ImVec2 window_size = ImVec2(viewport->Size.x, viewport->Size.y * 0.70f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###UIProgress", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 1.0f * globals::gui_scale));
        
        const float title_width = ImGui::CalcTextSize(str_title.c_str()).x;
        ImGui::SetCursorPosX(0.5f * (window_size.x - title_width));
        ImGui::TextUnformatted(str_title.c_str());

        ImGui::Dummy(ImVec2(0.0f, 8.0f * globals::gui_scale));

        if(!str_subtitle.empty()) {
            const float subtitle_width = ImGui::CalcTextSize(str_subtitle.c_str()).x;
            ImGui::SetCursorPosX(0.5f * (window_size.x - subtitle_width));
            ImGui::TextUnformatted(str_subtitle.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, 32.0f * globals::gui_scale));

        for(const Button &button : buttons) {
            const float button_width = 0.8f * ImGui::CalcItemWidth();
            ImGui::SetCursorPosX(0.5f * (window_size.x - button_width));

            if(ImGui::Button(button.str_title.c_str(), ImVec2(button_width, 0.0f))) {
                if(!button.action)
                    continue;
                button.action();
            }
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void message_box::reset(void)
{
    str_title.clear();
    str_subtitle.clear();
    buttons.clear();
}

void message_box::set_title(const std::string &title)
{
    str_title = language::resolve(title);
}

void message_box::set_subtitle(const std::string &subtitle)
{
    str_subtitle = language::resolve(subtitle);
}

void message_box::add_button(const std::string &text, const MessageBoxAction &action)
{
    Button button = {};
    button.str_title = fmt::format("{}###MessageBox_Button{}", language::resolve(text), buttons.size());
    button.action = action;

    buttons.push_back(button);
}
