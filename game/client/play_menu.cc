// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/fstools.hh>
#include <common/strtools.hh>
#include <cstdlib>
#include <deque>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/event/language_set.hh>
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

constexpr static ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

using ServerStatus = unsigned int;
constexpr static ServerStatus STATUS_INIT = 0x0000;
constexpr static ServerStatus STATUS_PING = 0x0001;
constexpr static ServerStatus STATUS_FAIL = 0x0002;
constexpr static ServerStatus STATUS_MOTD = 0x0003;

// Maximum amount of peers used for bothering
constexpr static std::size_t BOTHER_PEERS = 4;

// Default name for a server
// Mind you this is not translated
constexpr static const char *DEFAULT_SERVER_NAME = "Voxelius Server";

struct ServerStatusItem final {
    std::string peer_host {};
    std::uint16_t peer_port {};
    std::uint16_t max_players {};
    std::uint16_t num_players {};
    std::string server_motd {};
    ServerStatus status {};
    std::string name {};
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

static std::string input_itemname = {};
static std::string input_hostname = {};

static std::deque<ServerStatusItem *> servers_deque = {};
static ServerStatusItem *selected_server = nullptr;
static bool editing_server = false;
static bool adding_server = false;
static bool needs_focus = false;

static ENetHost *bother_host = nullptr;

static void parse_hostname(ServerStatusItem *item, const std::string &hostname)
{
    const std::vector<std::string> parts = strtools::split(hostname, ":");

    if(!parts[0].empty())
        item->peer_host = parts[0];
    else item->peer_host = std::string("localhost");
    
    if(parts.size() >= 2)
        item->peer_port = cxpr::clamp<std::uint16_t>(strtoul(parts[1].c_str(), nullptr, 10), 0x0000, UINT16_MAX);
    else item->peer_port = protocol::PORT;
}

static void add_new_server(void)
{
    ServerStatusItem *item = new ServerStatusItem();
    item->peer_port = protocol::PORT;
    item->max_players = UINT16_MAX;
    item->num_players = UINT16_MAX;
    item->server_motd = std::string();
    item->status = STATUS_FAIL;

    input_itemname = DEFAULT_SERVER_NAME;
    input_hostname = std::string();

    servers_deque.push_back(item);
    selected_server = item;
    editing_server = true;
    adding_server = true;
    needs_focus = true;
}

static void edit_selected_server(void)
{
    input_itemname = selected_server->name;
    if(selected_server->peer_port != protocol::PORT)
        input_hostname = fmt::format("{}:{}", selected_server->peer_host, selected_server->peer_port);
    else input_hostname = selected_server->peer_host;
    editing_server = true;
    needs_focus = true;
}

static void remove_selected_server(void)
{
    for(std::size_t i = 0; i < bother_host->peerCount; ++i) {
        if(bother_host->peers[i].data == static_cast<void *>(selected_server)) {
            enet_peer_reset(&bother_host->peers[i]);
            break;
        }
    }

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

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::gui_screen == GUI_PLAY_MENU) {
            if(editing_server) {
                if(adding_server)
                    remove_selected_server();
                input_itemname.clear();
                input_hostname.clear();
                editing_server = false;
                adding_server = false;
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

// NOTE: client-side networking doesn't attach
// anything to the peer's data field so we are free
// to assume anything with non-null data is a bother peer
static void on_status_response_packet(const protocol::StatusResponse &packet)
{
    if(packet.peer->data) {
        ServerStatusItem *item = reinterpret_cast<ServerStatusItem *>(packet.peer->data);
        item->max_players = packet.max_players;
        item->num_players = packet.num_players;
        item->server_motd = packet.motd;
        item->status = STATUS_MOTD;

        enet_peer_disconnect(packet.peer, 0);
    }
}

static void layout_server_item(ServerStatusItem *item)
{
    // Preserve the cursor at which we draw stuff
    const ImVec2 &cursor = ImGui::GetCursorScreenPos();
    const ImVec2 &padding = ImGui::GetStyle().FramePadding;
    const ImVec2 &spacing = ImGui::GetStyle().ItemSpacing;

    const float item_width = ImGui::GetContentRegionAvail().x;
    const float line_height = ImGui::GetTextLineHeightWithSpacing();
    const std::string sid = fmt::format("###play_menu.servers.{}", static_cast<void *>(item));
    if(ImGui::Selectable(sid.c_str(), (item == selected_server), 0, ImVec2(0.0, 2.0f * (line_height + padding.y + spacing.y)))) {
        selected_server = item;
        editing_server = false;
    }

    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        // Double clicked - join the selected server
        join_selected_server();
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    if(item == selected_server) {
        const ImVec2 start = ImVec2(cursor.x, cursor.y);
        const ImVec2 end = ImVec2(start.x + item_width, start.y + 2.0f * (line_height + padding.y + spacing.y));
        draw_list->AddRect(start, end, ImGui::GetColorU32(ImGuiCol_Text), 0.0f, 0, globals::gui_scale);
    }

    const ImVec2 name_pos = ImVec2(cursor.x + padding.x + 0.5f * spacing.x, cursor.y + padding.y);
    draw_list->AddText(name_pos, ImGui::GetColorU32(ImGuiCol_Text), item->name.c_str(), &item->name.cend()[0]);

    if(item->status == STATUS_MOTD) {
        const std::string stats = fmt::format("{}/{}", item->num_players, item->max_players);
        const float stats_width = ImGui::CalcTextSize(stats.c_str(), &stats.cend()[0]).x;
        const ImVec2 stats_pos = ImVec2(cursor.x + item_width - stats_width - padding.x, cursor.y + padding.y);
        draw_list->AddText(stats_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), stats.c_str(), &stats.cend()[0]);
    }

    ImU32 motd_color = {};
    const std::string *motd_text;

    switch(item->status) {
        case STATUS_INIT:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = &str_server_init;
            break;
        case STATUS_PING:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = &str_server_ping;
            break;
        case STATUS_MOTD:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = &item->server_motd;
            break;
        default:
            motd_color = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
            motd_text = &str_server_fail;
            break;
    }

    const ImVec2 motd_pos = ImVec2(cursor.x + padding.x + 0.5f * spacing.x, cursor.y + padding.y + line_height);
    draw_list->AddText(motd_pos, motd_color, motd_text->c_str(), &motd_text->cend()[0]);
}

static void layout_server_edit(ServerStatusItem *item)
{
    if(needs_focus) {
        ImGui::SetKeyboardFocusHere();
        needs_focus = false;
    }

    ImGui::SetNextItemWidth(-0.25f * ImGui::GetContentRegionAvail().x);
    ImGui::InputText("###play_menu.servers.edit_itemname", &input_itemname);
    ImGui::SameLine();

    const bool ignore_input = (strtools::is_empty_or_whitespace(input_itemname) || input_hostname.empty());
    ImGui::BeginDisabled(ignore_input);

    if(ImGui::Button("OK###play_menu.servers.submit_input", ImVec2(-1.0f, 0.0f)) || (!ignore_input && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        parse_hostname(item, input_hostname);
        item->name = input_itemname;
        item->status = STATUS_INIT;
        editing_server = false;
        adding_server = false;

        input_itemname.clear();
        input_hostname.clear();

        for(std::size_t i = 0; i < bother_host->peerCount; ++i) {
            if(bother_host->peers[i].data == static_cast<void *>(selected_server)) {
                enet_peer_reset(&bother_host->peers[i]);
                break;
            }
        }
    }

    ImGui::EndDisabled();

    ImGui::SetNextItemWidth(-0.25f * ImGui::GetContentRegionAvail().x);
    ImGui::InputText("###play_menu.servers.edit_hostname", &input_hostname, ImGuiInputTextFlags_CharsNoBlank);
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
            const auto parts = strtools::split(line, "%");
            ServerStatusItem *item = new ServerStatusItem();
            item->max_players = UINT16_MAX;
            item->num_players = UINT16_MAX;
            item->server_motd = std::string();
            item->status = STATUS_INIT;
            if(parts.size() >= 2)
                item->name = parts[1];
            else item->name = DEFAULT_SERVER_NAME;
            parse_hostname(item, parts[0]);
            servers_deque.push_back(item);
        }
    }

    bother_host = enet_host_create(nullptr, BOTHER_PEERS, 1, 0, 0);

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
        stream << fmt::format("{}:{}%{}", item->peer_host, item->peer_port, item->name) << std::endl;
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

    if(ImGui::Begin("###play_menu", nullptr, WINDOW_FLAGS)) {
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
    std::size_t num_free_peers = 0;

    for(std::size_t i = 0; i < bother_host->peerCount; ++i) {
        if(bother_host->peers[i].state != ENET_PEER_STATE_DISCONNECTED)
            continue;
        num_free_peers += 1;
    }

    for(ServerStatusItem *item : servers_deque) {
        if(num_free_peers && (item->status == STATUS_INIT)) {
            ENetAddress address = {};
            enet_address_set_host(&address, item->peer_host.c_str());
            address.port = item->peer_port;

            if(ENetPeer *peer = enet_host_connect(bother_host, &address, 1, 0)) {
                item->status = STATUS_PING;
                peer->data = item;
                break;
            }
        }
    }

    if(enet_host_service(bother_host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_CONNECT) {
            protocol::StatusRequest request = {};
            request.version = protocol::VERSION;
            protocol::send(event.peer, nullptr, request);
            return;
        }

        if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            ServerStatusItem *item = reinterpret_cast<ServerStatusItem *>(event.peer->data);
            if(item->status != STATUS_MOTD)
                item->status = STATUS_FAIL;
            return;
        }

        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            protocol::receive(event.packet, event.peer);
            enet_packet_destroy(event.packet);
            return;
        }
    }
}
