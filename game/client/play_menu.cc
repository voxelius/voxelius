// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/strtools.hh>
#include <cstdlib>
#include <deque>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/language_set.hh>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/play_menu.hh>
#include <game/shared/protocol.hh>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <mathlib/constexpr.hh>
#include <spdlog/fmt/fmt.h>

enum class ServerStatus : unsigned int {
    Init        = 0x0000, // The menu was just opened
    Pinging     = 0x0001, // Pinging the server
    Unreachable = 0x0002, // Can't connect or ping it
    Reachable   = 0x0003, // Has a MOTD and player stats
};

struct ServerStatusItem final {
    std::string server_host {};
    std::uint16_t server_port {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
    std::string server_motd {};
    ServerStatus status {};
};

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

static std::string str_worlds_tab = {};
static std::string str_servers_tab = {};
static std::string str_servers_add = {};
static std::string str_servers_edit = {};
static std::string str_servers_remove = {};
static std::string str_servers_join = {};
static std::string str_connect_tab = {};

static std::string input_hostname = {};
static std::deque<ServerStatusItem *> server_deque = {};
static ServerStatusItem *selected_server = nullptr;
static bool editing_server = false;

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::gui_screen == GUI_PLAY_MENU) {
            if(editing_server) {
                editing_server = false;
                return;
            }
            
            globals::gui_screen = GUI_MAIN_MENU;
            selected_server = nullptr;
            return;
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    str_worlds_tab = language::resolve_ui("play_menu.worlds");
    str_servers_tab = language::resolve_ui("play_menu.servers");
    str_servers_add = language::resolve_ui("play_menu.servers.add");
    str_servers_edit = language::resolve_ui("play_menu.servers.edit");
    str_servers_remove = language::resolve_ui("play_menu.servers.remove");
    str_servers_join = language::resolve_ui("play_menu.servers.join");
    str_connect_tab = language::resolve_ui("play_menu.connect");
}

static void add_new_server(void)
{
    ServerStatusItem *item = new ServerStatusItem();
    item->server_host = "localhost";
    item->server_port = protocol::PORT;
    item->max_players = UINT16_MAX;
    item->num_players = UINT16_MAX;
    item->server_motd = std::string();
    item->status = ServerStatus::Init;
    server_deque.push_back(item);
    selected_server = item;
    editing_server = true;
}

static void edit_selected_server(void)
{
    input_hostname = fmt::format("{}:{}", selected_server->server_host, selected_server->server_port);
    editing_server = true;
}

static void remove_selected_server(void)
{
    for(auto it = server_deque.cbegin(); it != server_deque.cend(); ++it) {
        if(selected_server == (*it)) {
            delete selected_server;
            server_deque.erase(it);
            selected_server = nullptr;
            return;
        }
    }
}

void play_menu::init(void)
{
    //server_deque.push_back(new ServerStatusItem());
    //server_deque.push_back(new ServerStatusItem());
    //server_deque.push_back(new ServerStatusItem());
    //server_deque.push_back(new ServerStatusItem());
    
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void play_menu::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(viewport->Size.x * 0.05f, viewport->Size.y * 0.05f);
    const ImVec2 window_size = ImVec2(viewport->Size.x * 0.90f, viewport->Size.y * 0.90f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###play_menu", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * globals::gui_scale, 3.0f * globals::gui_scale));

        if(ImGui::BeginTabBar("###play_menu.tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                globals::gui_screen = GUI_MAIN_MENU;
                selected_server = nullptr;
                editing_server = false;
            }

            if(ImGui::BeginTabItem(str_worlds_tab.c_str())) {
                if(ImGui::BeginChild("###play_menu.worlds.child")) {
                    ImGui::ShowStyleEditor();
                }
                
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_servers_tab.c_str())) {
                if(ImGui::BeginChild("###play_menu.servers.child", ImVec2(0.0f, -1.0f * ImGui::GetFrameHeightWithSpacing()))) {
                    if(ImGui::BeginListBox("###play_menu.servers.listbox", ImVec2(-1.0f, -1.0f))) {
                        for(ServerStatusItem *item : server_deque) {
                            if(editing_server && (item == selected_server)) {
                                ImGui::InputText("###OKINPUTBUDDY", &input_hostname, ImGuiInputTextFlags_CharsNoBlank);
                                ImGui::SameLine();
                                
                                if(ImGui::Button("OK###OKBUTTONBUDDY")) {
                                    const auto parts = strtools::split(input_hostname, ":");
                                    selected_server->server_host = parts[0].empty() ? "localhost" : parts[0];
                                    if(parts.size() >= 2)
                                        selected_server->server_port = cxpr::clamp<std::uint16_t>(strtoul(parts[1].c_str(), nullptr, 10), 0x0000, UINT16_MAX);
                                    else selected_server->server_port = protocol::PORT;
                                    editing_server = false;
                                    input_hostname.clear();
                                }

                                continue;
                            }
                            
                            const auto item_sid = fmt::format("###play_menu.servers.{}", static_cast<void *>(item));
                            if(ImGui::Selectable(item_sid.c_str(), (item == selected_server)))
                                selected_server = item;
                            ImGui::SameLine();
                            ImGui::TextDisabled("pinging...");
                            const std::string lx = std::string("42/64");
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::CalcTextSize(lx.c_str()).x - 2.0f * ImGui::GetStyle().FramePadding.x);
                            ImGui::TextDisabled(lx.c_str());
                        }
                        
                        ImGui::EndListBox();
                    }
                }
                
                ImGui::EndChild();
                
                const float button_width = ImGui::GetWindowSize().x / 4.0f - ImGui::GetStyle().FramePadding.x * 2.0f;
                
                ImGui::BeginDisabled(editing_server);
                if(ImGui::Button(str_servers_add.c_str(), ImVec2(button_width, 0.0f)))
                    add_new_server();
                ImGui::EndDisabled();
                ImGui::SameLine();

                // Allow these buttons to work only if there
                // is a selected server that is not being edited
                ImGui::BeginDisabled(!selected_server || editing_server);

                if(ImGui::Button(str_servers_edit.c_str(), ImVec2(button_width, 0.0f)))
                    edit_selected_server();
                ImGui::SameLine();

                if(ImGui::Button(str_servers_remove.c_str(), ImVec2(button_width, 0.0f)))
                    remove_selected_server();
                ImGui::SameLine();

                if(ImGui::Button(str_servers_join.c_str(), ImVec2(button_width, 0.0f)))
                    static_cast<void>(42);
                ImGui::SameLine();
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_connect_tab.c_str())) {
                if(ImGui::BeginChild("###play_menu.connect.child")) {
                    
                }
                
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}
