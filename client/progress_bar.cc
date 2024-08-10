// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/language.hh>
#include <client/progress_bar.hh>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

static std::string str_title = {};
static std::string str_button = {};
static ProgressBarAction button_action = {};
static float progress_value = {};

void progress_bar::init(void)
{
    str_title = "Loading";
    str_button = std::string();
    button_action = nullptr;
    progress_value = 0.0f;
}

void progress_bar::layout(void)
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

        const float bar_width = 0.8f * ImGui::CalcItemWidth();
        ImGui::SetCursorPosX(0.5f * (window_size.x - bar_width));
        ImGui::ProgressBar(progress_value, ImVec2(bar_width, 0.0f), "");

        if(!str_button.empty()) {
            ImGui::Dummy(ImVec2(0.0f, 32.0f * globals::gui_scale));

            const float button_width = 0.8f * ImGui::CalcItemWidth();
            ImGui::SetCursorPosX(0.5f * (window_size.x - button_width));

            if(ImGui::Button(str_button.c_str(), ImVec2(button_width, 0.0f))) {
                if(button_action) {
                    button_action();
                }
            }
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void progress_bar::reset(void)
{
    str_title.clear();
    str_button.clear();
    button_action = nullptr;
    progress_value = 0.0f;
}

void progress_bar::set_title(const std::string &title)
{
    str_title = language::resolve(title);
}

void progress_bar::set_button(const std::string &text, const ProgressBarAction &action)
{
    str_button = fmt::format("{}###ProgressBar_Button", language::resolve(text));
    button_action = action;
}

void progress_bar::set_progress(float value)
{
    progress_value = value;
}
