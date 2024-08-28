// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/server_list.hh>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/fmt/fmt.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;

static std::string str_title = "Server list";

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::gui_screen == GUI_SERVER_LIST) {
            globals::gui_screen = GUI_MAIN_MENU;
            return;
        }
    }
}

void server_list::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void server_list::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(viewport->Size.x * 0.05f, viewport->Size.y * 0.05f);
    const ImVec2 window_size = ImVec2(viewport->Size.x * 0.90f, viewport->Size.y * 0.90f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###server_list", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * globals::gui_scale, 3.0f * globals::gui_scale));

        if(ImGui::BeginTabBar("###server_list.tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                globals::gui_screen = GUI_MAIN_MENU;
                globals::gui_keybind_ptr = nullptr;
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(0.5f * (window_size.x - ImGui::CalcTextSize(str_title.c_str()).x));
            ImGui::TextUnformatted(str_title.c_str());
            
            if(ImGui::BeginChild("###server_list.child")) {
                if(ImGui::BeginTable("###server_list.table", 2)) {
                    ImGui::TableSetupColumn("###AAA", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("###BBB", ImGuiTableColumnFlags_WidthFixed);

                    for(int i = 0; i < 10; ++i) {
                        ImGui::TableNextRow();
                        ImGui::Selectable(fmt::format("###ITEM{}", i).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SetNextItemWidth(-1);
                        ImGui::Text("Something\nthat spans multiple lines");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextDisabled("4/16");
                    }

                    ImGui::EndTable();
                }
            }

            ImGui::EndChild();
            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}
