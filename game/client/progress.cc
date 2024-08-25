// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/globals.hh>
#include <game/client/language.hh>
#include <game/client/progress.hh>
#include <imgui.h>
#include <mathlib/constexpr.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

static std::string str_title = {};
static std::string str_button = {};
static ProgressBarAction button_action = {};

void progress::init(void)
{
    str_title = "Loading";
    str_button = std::string();
    button_action = nullptr;
}

void progress::layout(void)
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

        const ImVec2 cursor = ImGui::GetCursorPos();

        const std::size_t num_bars = 32;
        const float spinner_width = 0.8f * ImGui::CalcItemWidth();
        const float bar_width = spinner_width / static_cast<float>(num_bars);
        const float bar_height = 0.5f * ImGui::GetFrameHeight();
        
        const float base_xpos = window_start.x + 0.5f * (window_size.x - spinner_width) + 0.5f;
        const float base_ypos = window_start.y + cursor.y;
        const float phase = 2.0f * ImGui::GetTime();

        const ImVec4 &background = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 &foreground = ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);

        for(std::size_t i = 0; i < num_bars; ++i) {
            const float sinval = std::sin(M_PI * static_cast<float>(i) / static_cast<float>(num_bars) - phase);
            const float modifier = std::exp(-8.0f * (0.5f + 0.5f * sinval));

            ImVec4 color = {};
            color.x = cxpr::lerp(background.x, foreground.x, modifier);
            color.y = cxpr::lerp(background.y, foreground.y, modifier);
            color.z = cxpr::lerp(background.z, foreground.z, modifier);
            color.w = cxpr::lerp(background.w, foreground.w, modifier);
            
            const ImVec2 start = ImVec2(base_xpos + bar_width * i, base_ypos);
            const ImVec2 end = ImVec2(start.x + bar_width, start.y + bar_height);
            ImGui::GetWindowDrawList()->AddRectFilled(start, end, ImGui::GetColorU32(color));
        }

        // The NewLine call tricks ImGui into correctly padding the
        // next widget that comes after the progress spinner; this
        // is needed to ensure the button is located in the correct place
        ImGui::NewLine();

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

void progress::reset(void)
{
    str_title.clear();
    str_button.clear();
    button_action = nullptr;
}

void progress::set_title(const std::string &title)
{
    str_title = language::resolve(title);
}

void progress::set_button(const std::string &text, const ProgressBarAction &action)
{
    str_button = fmt::format("{}###ProgressBar_Button", language::resolve(text));
    button_action = action;
}
