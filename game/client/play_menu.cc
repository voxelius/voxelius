// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/fstools.hh>
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
#include <game/client/session.hh>
#include <game/shared/protocol.hh>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <mathlib/constexpr.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <sstream>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

using ServerStatus = unsigned int;
constexpr static ServerStatus STATUS_INIT = 0x0000;
constexpr static ServerStatus STATUS_PING = 0x0001;
constexpr static ServerStatus STATUS_FAIL = 0x0002;
constexpr static ServerStatus STATUS_MOTD = 0x0003;

struct ServerStatusItem final {
    std::string peer_host {};
    std::uint16_t peer_port {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
    std::string server_motd {};
    ServerStatus status {};
};

static std::string str_worlds_tab = {};
static std::string str_servers_tab = {};
static std::string str_servers_join = {};
static std::string str_servers_connect = {};
static std::string str_servers_add = {};
static std::string str_servers_edit = {};
static std::string str_servers_remove = {};
static std::string str_servers_refresh = {};

static std::string str_server_init = {};
static std::string str_server_ping = {};
static std::string str_server_fail = {};

static std::string input_hostname = {};
static std::deque<ServerStatusItem *> servers_deque = {};
static ServerStatusItem *selected_server = nullptr;
static bool editing_server = false;
static bool needs_focus = false;

static ENetHost *bother_host = nullptr;

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
    str_servers_join = language::resolve_ui("play_menu.servers.join");
    str_servers_connect = language::resolve_ui("play_menu.servers.connect");
    str_servers_add = language::resolve_ui("play_menu.servers.add");
    str_servers_edit = language::resolve_ui("play_menu.servers.edit");
    str_servers_remove = language::resolve_ui("play_menu.servers.remove");
    str_servers_refresh = language::resolve_ui("play_menu.servers.refresh");

    str_server_init = language::resolve("play_menu.server.init");
    str_server_ping = language::resolve("play_menu.server.ping");
    str_server_fail = language::resolve("play_menu.server.fail");
}

static void on_status_response_packet(const protocol::StatusResponse &packet)
{
    if(packet.peer->data) {
        ServerStatusItem *item = reinterpret_cast<ServerStatusItem *>(packet.peer->data);
        item->max_players = packet.max_players;
        item->num_players = packet.num_players;
        item->server_motd = fmt::format("Protocol version: {}", packet.version);
        item->status = STATUS_MOTD;

        enet_peer_disconnect(packet.peer, 0);
    }
}

static void submit_server_item(ServerStatusItem *item, const std::string &hostname)
{
    const std::vector<std::string> parts = strtools::split(hostname, ":");

    if(!parts[0].empty())
        item->peer_host = parts[0];
    else item->peer_host = std::string("localhost");
    
    if(parts.size() >= 2)
        item->peer_port = cxpr::clamp<std::uint16_t>(strtoul(parts[1].c_str(), nullptr, 10), 0x0000, UINT16_MAX);
    else item->peer_port = protocol::PORT;
}

static void layout_server_item(ServerStatusItem *item)
{
    const std::string sid = fmt::format("###play_menu.servers.{}", static_cast<void *>(item));
    
    if(ImGui::Selectable(sid.c_str(), (item == selected_server))) {
        selected_server = item;
        editing_server = false;
    }

    ImGui::SameLine();

    switch(item->status) {
        case STATUS_INIT: ImGui::TextDisabled(str_server_init.c_str()); break;
        case STATUS_PING: ImGui::TextDisabled(str_server_ping.c_str()); break;
        case STATUS_FAIL: ImGui::TextDisabled(str_server_fail.c_str()); break;
        case STATUS_MOTD: ImGui::TextUnformatted(item->server_motd.c_str()); break;
    }

    if(item->status == STATUS_MOTD) {
        ImGui::SameLine();
        const auto str = fmt::format("{}/{}", item->num_players, item->max_players);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::CalcTextSize(str.c_str()).x - 2.0f * ImGui::GetStyle().FramePadding.x);
        ImGui::TextDisabled(str.c_str());
    }
}

static void layout_server_edit(ServerStatusItem *item)
{
    if(needs_focus) {
        ImGui::SetKeyboardFocusHere();
        needs_focus = false;
    }

    ImGui::SetNextItemWidth(-0.25f * ImGui::GetContentRegionAvail().x);
    ImGui::InputText("###play_menu.servers.edit_input", &input_hostname, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    
    if(ImGui::Button("OK###play_menu.servers.submit_input", ImVec2(-1.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        submit_server_item(item, input_hostname);
        item->status = STATUS_INIT;
        editing_server = false;
        input_hostname.clear();
    }
}

static void add_new_server(void)
{
    ServerStatusItem *item = new ServerStatusItem();
    item->peer_host = std::string("localhost");
    item->peer_port = protocol::PORT;
    item->max_players = UINT16_MAX;
    item->num_players = UINT16_MAX;
    item->server_motd = std::string();
    item->status = STATUS_FAIL;

    servers_deque.push_back(item);
    selected_server = item;
    editing_server = true;
    needs_focus = true;
}

static void edit_selected_server(void)
{
    if(selected_server->peer_port != protocol::PORT)
        input_hostname = fmt::format("{}:{}", selected_server->peer_host, selected_server->peer_port);
    else input_hostname = selected_server->peer_host;
    editing_server = true;
    needs_focus = true;
}

static void remove_selected_server(void)
{
    for(auto it = servers_deque.cbegin(); it != servers_deque.cend(); ++it) {
        if(selected_server == (*it)) {
            delete selected_server;
            selected_server = nullptr;
            servers_deque.erase(it);
            return;
        }
    }
}

static void join_selected_server(void)
{
    if(globals::session_peer)
        return;
    session::connect(selected_server->peer_host, selected_server->peer_port);
}

static void layout_worlds(void)
{
}

static void layout_servers(void)
{
    if(ImGui::BeginListBox("###play_menu.servers.listbox", ImVec2(-1.0f, -1.0f))) {
        for(ServerStatusItem *item : servers_deque) {
            if(editing_server && (item == selected_server))
                layout_server_edit(item);
            else layout_server_item(item);
        }
        
        ImGui::EndListBox();
    }
}

static void layout_servers_buttons(void)
{
    const float avail_width = ImGui::GetContentRegionAvail().x;

    // Can only join when selected and not editing
    ImGui::BeginDisabled(!selected_server || editing_server);
    if(ImGui::Button(str_servers_join.c_str(), ImVec2(-0.50f * avail_width, 0.0f)))
        join_selected_server();
    ImGui::EndDisabled();
    ImGui::SameLine();
    
    // Can only connect directly when not editing anything
    ImGui::BeginDisabled(editing_server);
    if(ImGui::Button(str_servers_connect.c_str(), ImVec2(-1.00f, 0.0f)))
        spdlog::debug("UNDONE: direct connect is not implemented!");
    ImGui::EndDisabled();
    
    // Can only add when not editing anything
    ImGui::BeginDisabled(editing_server);
    if(ImGui::Button(str_servers_add.c_str(), ImVec2(-0.75f * avail_width, 0.0f)))
        add_new_server();
    ImGui::EndDisabled();
    ImGui::SameLine();

    // Can only edit when selected and not editing
    ImGui::BeginDisabled(!selected_server || editing_server);
    if(ImGui::Button(str_servers_edit.c_str(), ImVec2(-0.50f * avail_width, 0.0f)))
        edit_selected_server();
    ImGui::EndDisabled();
    ImGui::SameLine();

    // Can only remove when selected and not editing
    ImGui::BeginDisabled(!selected_server || editing_server);
    if(ImGui::Button(str_servers_remove.c_str(), ImVec2(-0.25f * avail_width, 0.0f)))
        remove_selected_server();
    ImGui::EndDisabled();
    ImGui::SameLine();

    if(ImGui::Button(str_servers_refresh.c_str(), ImVec2(-1.0f, 0.0f))) {
        for(ServerStatusItem *item : servers_deque) {
            if(item->status != STATUS_PING) {
                if(editing_server && (item == selected_server))
                    continue;
                item->status = STATUS_INIT;
            }
        }
    }
}

static void layout_connect(void)
{
}

void play_menu::init(void)
{
    std::string source = {};

    if(fstools::read_string("servers.txt", source)) {
        std::istringstream stream = std::istringstream(source);
        std::string line;
        
        while(std::getline(stream, line)) {
            ServerStatusItem *item = new ServerStatusItem();
            item->max_players = UINT16_MAX;
            item->num_players = UINT16_MAX;
            item->server_motd = std::string();
            item->status = STATUS_INIT;
            submit_server_item(item, line);
            servers_deque.push_back(item);
        }
    }

    bother_host = enet_host_create(nullptr, 4, 1, 0, 0);

    if(!bother_host) {
        spdlog::warn("play_menu: cannot create a bother ENetHost");
        spdlog::warn("play_menu: this is not a death scenario but server status will not be available");
    }

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
    globals::dispatcher.sink<protocol::StatusResponse>().connect<&on_status_response_packet>();
}

void play_menu::deinit(void)
{
    std::ostringstream stream = {};
    for(ServerStatusItem *item : servers_deque)
        stream << fmt::format("{}:{}", item->peer_host, item->peer_port) << std::endl;
    fstools::write_string("servers.txt", stream.str());

    for(ServerStatusItem *item : servers_deque)
        delete item;
    servers_deque.clear();

    enet_host_destroy(bother_host);
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
                if(ImGui::BeginChild("###play_menu.worlds.child"))
                    layout_worlds();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_servers_tab.c_str())) {
                if(ImGui::BeginChild("###play_menu.servers.child", ImVec2(0.0f, -2.0f * ImGui::GetFrameHeightWithSpacing())))
                    layout_servers();
                ImGui::EndChild();
                layout_servers_buttons();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void play_menu::update_late(void)
{
    ENetEvent event = {};

    while(enet_host_service(bother_host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_CONNECT) {
            protocol::StatusRequest request = {};
            request.version = protocol::VERSION;
            protocol::send(event.peer, nullptr, request);
            continue;
        }

        if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            ServerStatusItem *item = reinterpret_cast<ServerStatusItem *>(event.peer->data);
            if(item->status != STATUS_MOTD)
                item->status = STATUS_FAIL;
            continue;
        }

        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            protocol::receive(event.packet, event.peer);
            enet_packet_destroy(event.packet);
            continue;
        }
    }

    for(ServerStatusItem *item : servers_deque) {
        if(item->status == STATUS_INIT) {
            ENetAddress address = {};
            enet_address_set_host(&address, item->peer_host.c_str());
            address.port = item->peer_port;

            // This will fail whenever there's not enough free peer slots
            if(ENetPeer *peer = enet_host_connect(bother_host, &address, 1, 0)) {
                item->status = STATUS_PING;
                peer->data = item;
                continue;
            }
        }
    }
}
