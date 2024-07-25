// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/globals.hh>
#include <client/lang.hh>
#include <client/ui_progress.hh>
#include <glm/common.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = (
    ImGuiWindowFlags_NoBackground       |
    ImGuiWindowFlags_NoCollapse         |
    ImGuiWindowFlags_NoMove             |
    ImGuiWindowFlags_NoResize           |
    ImGuiWindowFlags_NoSavedSettings    |
    ImGuiWindowFlags_NoTitleBar
);

static std::string progress_title = {};
static std::string progress_subtitle = {};
static float progress_progress = {};

void ui::progress::init(void)
{
    progress_title = "Loading";
    progress_subtitle = "Doing stuff";
    progress_progress = 0.0f;
}

void ui::progress::layout(void)
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(0.0f, viewport->Size.y * 0.30f);
    const ImVec2 window_size = ImVec2(viewport->Size.x, viewport->Size.y * 0.70f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###UIProgress", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f * globals::ui_scale));
        
        ImGui::PushFont(globals::font_menu_button);
        const float title_width = ImGui::CalcTextSize(progress_title.c_str()).x;
        ImGui::SetCursorPosX(0.5f * (window_size.x - title_width));
        ImGui::TextUnformatted(progress_title.c_str());
        ImGui::PopFont();

        if(!progress_subtitle.empty()) {
            ImGui::Spacing();
            const float subtitle_width = ImGui::CalcTextSize(progress_subtitle.c_str()).x;
            ImGui::SetCursorPosX(0.5f * (window_size.x - subtitle_width));
            ImGui::TextUnformatted(progress_subtitle.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, 16.0f * globals::ui_scale));

        const float bar_width = 240.0f * globals::ui_scale;
        ImGui::SetCursorPosX(0.5f * (window_size.x - bar_width));
        ImGui::ProgressBar(progress_progress, ImVec2(bar_width, 0.0f), "");

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void ui::progress::set_title(const std::string &title)
{
    progress_title = lang::resolve(title);
}

void ui::progress::set_subtitle(const std::string &subtitle)
{
    progress_subtitle = lang::resolve(subtitle);
}

void ui::progress::set_progress(float progress)
{
    progress_progress = progress;
}
