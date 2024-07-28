// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/lang.hh>
#include <client/progress.hh>
#include <imgui.h>
#include <imgui_stdlib.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

static std::string str_title = {};
static std::string str_subtitle = {};
static float progress_value = {};

void progress::init(void)
{
    str_title = "Loading";
    str_subtitle = std::string();
    progress_value = 0.0f;
}

void progress::layout(void)
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(0.0f, viewport->Size.y * 0.30f);
    const ImVec2 window_size = ImVec2(viewport->Size.x, viewport->Size.y * 0.70f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###UIProgress", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f * globals::ui_scale));
        
        ImGui::PushFont(globals::font_menu_button);
        const float title_width = ImGui::CalcTextSize(str_title.c_str()).x;
        ImGui::SetCursorPosX(0.5f * (window_size.x - title_width));
        ImGui::TextUnformatted(str_title.c_str());
        ImGui::PopFont();

        if(!str_subtitle.empty()) {
            ImGui::Spacing();
            const float subtitle_width = ImGui::CalcTextSize(str_subtitle.c_str()).x;
            ImGui::SetCursorPosX(0.5f * (window_size.x - subtitle_width));
            ImGui::TextUnformatted(str_subtitle.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, 16.0f * globals::ui_scale));

        const float bar_width = 240.0f * globals::ui_scale;
        ImGui::SetCursorPosX(0.5f * (window_size.x - bar_width));
        ImGui::ProgressBar(progress_value, ImVec2(bar_width, 0.0f), "");

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void progress::set_title(const std::string &title)
{
    str_title = lang::resolve(title);
}

void progress::set_subtitle(const std::string &subtitle)
{
    str_subtitle = lang::resolve(subtitle);
}

void progress::set(float value)
{
    progress_value = value;
}
