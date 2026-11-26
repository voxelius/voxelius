#include "client/pch.hh"

#include "client/play_menu.hh"

#include "core/exception.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"
#include "core/version.hh"

#include "shared/net/packet_session.hh"
#include "shared/net/protocol.hh"

#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/gui/background.hh"
#include "client/gui/button.hh"
#include "client/gui/container.hh"
#include "client/gui/label.hh"
#include "client/gui/list_box.hh"
#include "client/gui/menu_button.hh"
#include "client/gui/panel.hh"
#include "client/gui/popup.hh"
#include "client/gui/rail.hh"
#include "client/gui/screen.hh"
#include "client/gui/stack.hh"
#include "client/gui/stone.hh"
#include "client/language.hh"
#include "client/loading.hh"
#include "client/main_menu.hh"
#include "client/net/bother.hh"
#include "client/net/session.hh"

constexpr static float ROW_HEIGHT = 14.0f;
constexpr static float BODY_MARGIN = 16.0f;
constexpr static std::string_view DEFAULT_NAME = "Voxelius Server";
constexpr static std::string_view JSON_PATH = "servers.json";

constexpr static std::size_t MIN_ROW_LINES = 3;
constexpr static std::size_t MAX_MOTD_LINES = 4;
class ServerListItem final : public gui::ListBoxItemBuilder<ServerListItem> {
public:
    constexpr static unsigned UNKNOWN = 0;
    constexpr static unsigned PINGING = 1;
    constexpr static unsigned REACHED = 2;
    constexpr static unsigned UNREACHABLE = 3;

    virtual void layout(void) override;

    void invalidate(void);

    unsigned status { UNKNOWN };
    unsigned identity;
    bool configured { false };

    std::string name;
    std::string host;
    std::uint16_t port;
    std::uint64_t invite;

    std::uint16_t num_players { 0 };
    std::uint16_t max_players { 0 };
    std::uint32_t version_major { 0 };
    std::uint32_t version_minor { 0 };
    std::uint32_t version_patch { 0 };
    std::string motd;

    std::optional<std::string> stats_string {};
    std::optional<std::string> version_string {};
};

static std::string s_motd_init {};
static std::string s_motd_ping {};
static std::string s_motd_fail {};

static std::deque<std::unique_ptr<ServerListItem>> s_servers;
static vx::hash_map<unsigned, ServerListItem*> s_server_map;
static unsigned s_next_identity;

static gui::ListBox s_server_list;
static ServerListItem* s_selected_server;

static gui::Button s_btn_join;
static gui::MenuButton s_btn_connect;
static gui::Button s_btn_add;
static gui::Button s_btn_edit;
static gui::Button s_btn_remove;
static gui::Button s_btn_refresh;
static gui::HorizontalStack s_servers_action_row;

static gui::Background s_background;
static gui::Panel s_body;
static gui::Rail s_rail;

static gui::Label s_worlds_todo;
static gui::Container s_worlds_tab;
static gui::VerticalStack s_servers_tab;
static gui::InputPopup s_server_popup;
static gui::InputPopup s_direct_popup;
static gui::ChoicePopup s_error_popup;

gui::Screen play_menu::screen;

static std::vector<std::string> wrap_text(std::string_view text, float max_width, std::size_t max_lines)
{
    std::vector<std::string> lines;
    std::string current;

    std::size_t index = 0;

    while(index < text.size()) {
        auto space = text.find(' ', index);
        auto end = space;

        if(space == std::string_view::npos) {
            end = text.size();
        }

        auto word = text.substr(index, end - index);

        std::string candidate;
        if(current.empty()) {
            candidate = std::string(word);
        }
        else {
            candidate = current + " " + std::string(word);
        }

        auto candidate_size = ImGui::CalcTextSize(candidate.c_str());

        if(current.empty() || candidate_size.x <= max_width) {
            current = std::move(candidate);
        }
        else {
            lines.push_back(std::move(current));
            current = std::string(word);
        }

        if(space == std::string_view::npos) {
            break;
        }

        index = space + 1;
    }

    if(!current.empty()) {
        lines.push_back(std::move(current));
    }

    if(lines.size() > max_lines) {
        lines.resize(max_lines);
        lines.back() += "...";
    }

    return lines;
}

static void select_server(ServerListItem* item)
{
    if(s_selected_server) {
        s_selected_server->set_selected(false);
    }

    s_selected_server = item;

    if(s_selected_server) {
        s_selected_server->set_selected(true);
    }

    s_btn_join.set_enabled(s_selected_server);
    s_btn_edit.set_enabled(s_selected_server);
    s_btn_remove.set_enabled(s_selected_server);
}

static void toggle_server(ServerListItem* item)
{
    select_server((s_selected_server == item) ? nullptr : item);
}

static void join_server(ServerListItem* item)
{
    session::connect(item->host, item->port, item->invite);
}

static ServerListItem* create_server(void)
{
    auto owned = std::make_unique<ServerListItem>();
    auto item = owned.get();

    item->identity = s_next_identity;
    s_next_identity += 1;

    item->on_single_click([item] {
        toggle_server(item);
    });

    item->on_double_click([item] {
        join_server(item);
    });

    s_server_map.emplace(item->identity, item);
    s_server_list.add_child(*item);
    s_servers.push_back(std::move(owned));

    return item;
}

static void destroy_server(ServerListItem* item)
{
    if(s_selected_server == item) {
        select_server(nullptr);
    }

    bother::cancel(item->identity);

    s_server_list.remove_child(*item);
    s_server_map.erase(item->identity);

    std::erase_if(s_servers, [item](const std::unique_ptr<ServerListItem>& owned) {
        return item == owned.get();
    });
}

static std::pair<std::string_view, std::uint16_t> parse_hostname(std::string_view hostname)
{
    auto colon = hostname.find(':');

    if(colon == std::string_view::npos) {
        if(hostname.empty()) {
            return std::make_pair(std::string_view("localhost"), protocol::PORT);
        }
        else {
            return std::make_pair(hostname, protocol::PORT);
        }
    }
    else {
        auto host_part = hostname.substr(0, colon);
        auto port_part = hostname.substr(colon + 1);

        std::uint16_t port;
        auto check = std::from_chars(port_part.data(), port_part.data() + port_part.size(), port);

        if(check.ec == std::errc {}) {
            return std::make_pair(host_part, std::max<std::uint16_t>(1024, port));
        }
        else {
            return std::make_pair(host_part, protocol::PORT);
        }
    }
}

static void apply_popup_values(ServerListItem* item, std::span<const std::string> values)
{
    auto parts = parse_hostname(values[1]);
    auto name = values[0].substr(0, 18);

    if(utils::is_whitespace(name)) {
        item->name = DEFAULT_NAME;
    }
    else {
        item->name = name;
    }

    item->host = parts.first;
    item->port = parts.second;
    item->invite = 0;
    item->status = ServerListItem::UNKNOWN;
    item->configured = true;

    if(values[2].size()) {
        std::uint64_t invite;
        auto check = std::from_chars(values[2].data(), values[2].data() + values[2].size(), invite);

        if(check.ec == std::errc {}) {
            item->invite = invite;
        }
    }

    item->invalidate();
}

static void open_add_popup(void)
{
    auto item = create_server();
    item->port = protocol::PORT;

    s_server_popup.set_value(0, DEFAULT_NAME);
    s_server_popup.set_value(1, {});
    s_server_popup.set_value(2, {});

    s_server_popup.on_submit([item](std::span<const std::string> values) {
        apply_popup_values(item, values);
    });

    s_server_popup.on_cancel([item] {
        destroy_server(item);
    });

    s_server_popup.open();

    select_server(item);
}

static void open_edit_popup(void)
{
    if(s_selected_server) {
        auto item = s_selected_server;
        std::string hostname;

        if(item->port == protocol::PORT) {
            hostname = item->host;
        }
        else {
            hostname = std::format("{}:{}", item->host, item->port);
        }

        s_server_popup.set_value(0, item->name);
        s_server_popup.set_value(1, hostname);

        if(item->invite) {
            s_server_popup.set_value(2, std::to_string(item->invite));
        }
        else {
            s_server_popup.set_value(2, {});
        }

        s_server_popup.on_submit([item](std::span<const std::string> values) {
            apply_popup_values(item, values);
        });

        s_server_popup.on_cancel({});

        s_server_popup.open();
    }
}

static void refresh_servers(void)
{
    for(auto& owned : s_servers) {
        bother::cancel(owned->identity);
        owned->status = ServerListItem::UNKNOWN;
        owned->invalidate();
    }
}

static void on_bother_response(const BotherResponseEvent& event)
{
    auto it = s_server_map.find(event.request_id());

    if(it == s_server_map.end()) {
        return;
    }

    auto item = it->second;

    if(event.unreachable()) {
        item->status = ServerListItem::UNREACHABLE;
    }
    else {
        item->status = ServerListItem::REACHED;
        item->num_players = event.num_players();
        item->max_players = event.max_players();
        item->version_major = event.version_major();
        item->version_minor = event.version_minor();
        item->version_patch = event.version_patch();
        item->motd = event.motd();
    }

    item->invalidate();
}

static void on_language_update(const LanguageUpdateEvent&)
{
    play_menu::screen.translate();

    s_motd_init = language::resolve("play_menu.servers.motd_init");
    s_motd_ping = language::resolve("play_menu.servers.motd_ping");
    s_motd_fail = language::resolve("play_menu.servers.motd_fail");
}

static void on_keyboard_event(const SDL_KeyboardEvent& event)
{
    auto should_close = true;
    should_close = should_close && event.type == SDL_EVENT_KEY_DOWN && event.key == SDLK_ESCAPE;
    should_close = should_close && globals::gui_screen == &play_menu::screen;
    should_close = should_close && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

    if(should_close) {
        main_menu::show();
    }
}

static void on_session_state(const SessionStateEvent& event)
{
    switch(event.state()) {
        case SESSION_CONNECTING:
            loading::show_stage(LOADING_CONNECTING);
            break;

        case SESSION_AUTHENTICATING:
            loading::show_stage(LOADING_AUTHENTICATING);
            break;

        case SESSION_SPAWNING:
            loading::show_stage(LOADING_SPAWNING);
            break;

        case SESSION_INGAME:
        case SESSION_DISCONNECTED:
            // The loading screen switches itself on the player
            // assignment and closes itself on a disconnect
            break;
    }
}

static void on_session_error(const SessionErrorEvent& event)
{
    play_menu::show_error(packet::Session_Disconnect::reason_string_client(event.reason()));
}

static void load_servers_json(void)
{
    std::string source;

    if(utils::read_file(JSON_PATH, source)) {
        auto jsonv = json_parse_string(source.c_str());
        auto json = json_value_get_object(jsonv);

        if(jsonv == nullptr) {
            msg::log_warning("{}: malformed JSON", JSON_PATH);
            return;
        }

        if(json == nullptr) {
            msg::log_warning("{}: expected JSON object", JSON_PATH);
            json_value_free(jsonv);
            return;
        }

        auto direct_hostname = json_object_get_string(json, "direct_hostname");
        auto direct_invite_str = json_object_get_string(json, "direct_invite");
        auto servers = json_object_get_array(json, "servers");

        if(direct_hostname == nullptr || direct_invite_str == nullptr || servers == nullptr) {
            msg::log_warning("{}: expected JSON object with direct_hostname, direct_invite, and servers keys", JSON_PATH);
            json_value_free(jsonv);
            return;
        }

        std::uint64_t direct_invite {};
        auto direct_invite_size = std::strlen(direct_invite_str);
        auto direct_invite_check = std::from_chars(direct_invite_str, direct_invite_str + direct_invite_size, direct_invite);

        if(direct_invite_check.ec == std::errc {}) {
            s_direct_popup.set_value(0, direct_hostname);
            s_direct_popup.set_value(1, std::to_string(direct_invite));
        }

        auto count = json_array_get_count(servers);

        for(std::size_t i = 0; i < count; ++i) {
            auto item = json_array_get_object(servers, i);
            auto name = json_object_get_string(item, "name");
            auto hostname = json_object_get_string(item, "hostname");
            auto invite_str = json_object_get_string(item, "invite");

            auto item_ok = item && name;
            if(!item_ok || hostname == nullptr || invite_str == nullptr) {
                msg::log_warning("{}: expected JSON object with name, hostname, and invite keys", JSON_PATH);
                continue;
            }

            std::uint64_t invite {};
            auto invite_size = std::strlen(invite_str);
            auto invite_check = std::from_chars(invite_str, invite_str + invite_size, invite);

            if(invite_check.ec == std::errc {}) {
                auto parts = parse_hostname(hostname);
                auto server = create_server();
                server->name = name;
                server->host = parts.first;
                server->port = parts.second;
                server->invite = invite;
                server->status = ServerListItem::UNKNOWN;
                server->configured = true;
                server->invalidate();
            }
        }

        json_value_free(jsonv);
    }
}

static void save_servers_json(void)
{
    auto jsonv = json_value_init_object();
    auto json = json_value_get_object(jsonv);
    vx::throw_if(jsonv == nullptr || json == nullptr);

    auto direct_hostname = std::string(s_direct_popup.value(0));
    auto direct_invite = std::string(s_direct_popup.value(1));

    json_object_set_string(json, "direct_hostname", direct_hostname.c_str());
    json_object_set_string(json, "direct_invite", direct_invite.c_str());

    auto serversv = json_value_init_array();
    auto servers = json_value_get_array(serversv);
    vx::throw_if(serversv == nullptr || servers == nullptr);

    for(auto& owned : s_servers) {
        if(owned->configured) {
            auto itemv = json_value_init_object();
            auto item = json_value_get_object(itemv);
            vx::throw_if(itemv == nullptr || item == nullptr);

            auto hostname = std::format("{}:{}", owned->host, owned->port);
            auto invite_str = std::to_string(owned->invite);

            json_object_set_string(item, "name", owned->name.c_str());
            json_object_set_string(item, "hostname", hostname.c_str());
            json_object_set_string(item, "invite", invite_str.c_str());

            json_array_append_value(servers, itemv);
        }
    }

    json_object_set_value(json, "servers", serversv);

    std::string source;
    source.resize(json_serialization_size(jsonv));
    json_serialize_to_buffer(jsonv, source.data(), source.size());

    utils::write_file(JSON_PATH, source);

    json_value_free(jsonv);
}

void ServerListItem::layout(void)
{
    const auto& style = ImGui::GetStyle();
    const auto& padding = style.FramePadding;
    const auto& spacing = style.ItemSpacing;

    ImU32 motd_color {};
    std::string_view motd_text {};

    switch(status) {
        case UNKNOWN:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = s_motd_init;
            break;

        case PINGING:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = s_motd_ping;
            break;

        case REACHED:
            motd_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            motd_text = motd;
            break;

        default:
            motd_color = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
            motd_text = s_motd_fail;
            break;
    }

    auto item_size = ImGui::GetContentRegionAvail();
    auto line_height = ImGui::GetTextLineHeightWithSpacing();

    auto text_width = std::max(1.0f, item_size.x - 2.0f * padding.x - spacing.x);
    auto motd_lines = wrap_text(motd_text, text_width, MAX_MOTD_LINES);

    auto rows = std::max<std::size_t>(MIN_ROW_LINES, 1 + motd_lines.size());
    auto row_height = static_cast<float>(rows) * (line_height + padding.y + spacing.y);

    auto cursor_pos = this->begin_row(ImVec2(0.0f, row_height));
    auto draw_list = ImGui::GetWindowDrawList();

    ImVec2 name_pos {};
    name_pos.x = cursor_pos.x + padding.x + 0.5f * spacing.x;
    name_pos.y = cursor_pos.y + padding.y;

    auto text_shadow_color = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.750f));
    auto text_shadow_offset = gui::detail::text_shadow_offset();

    auto name_shadow_pos = ImVec2(name_pos.x + text_shadow_offset, name_pos.y + text_shadow_offset);
    auto name_color = ImGui::GetColorU32(ImGuiCol_Text);

    draw_list->AddText(name_shadow_pos, text_shadow_color, name.c_str(), name.c_str() + name.size());
    draw_list->AddText(name_pos, name_color, name.c_str(), name.c_str() + name.size());

    if(status == REACHED) {
        if(!stats_string.has_value()) {
            stats_string = std::format("{}/{}", num_players, max_players);
        }

        auto stats_cbegin = stats_string->data();
        auto stats_cend = stats_cbegin + stats_string->size();
        auto stats_size = ImGui::CalcTextSize(stats_cbegin, stats_cend);

        ImVec2 stats_pos {};
        stats_pos.x = cursor_pos.x + item_size.x - stats_size.x - padding.x - 0.5f * spacing.x;
        stats_pos.y = cursor_pos.y + padding.y;

        auto stats_shadow_pos = ImVec2(stats_pos.x + text_shadow_offset, stats_pos.y + text_shadow_offset);
        auto stats_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        draw_list->AddText(stats_shadow_pos, text_shadow_color, stats_cbegin, stats_cend);
        draw_list->AddText(stats_pos, stats_color, stats_cbegin, stats_cend);

        auto major_match = version_major == version::major;
        auto minor_match = version_minor == version::minor;
        auto patch_match = version_patch == version::patch;

        ImU32 version_color {};

        if(major_match && minor_match && patch_match) {
            version_color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        }
        else if(major_match && minor_match) {
            version_color = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
        }
        else {
            version_color = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
        }

        if(!version_string.has_value()) {
            // NOTE: we're not doing the 16.x.x thing
            // and not bothering with custom diplay formats
            // now since 16.x.x and 17.x.x are fundamentally different
            version_string = std::format("{}.{}.{}", version_major, version_minor, version_patch);
        }

        ImGui::PushFont(fonts::unscii8, 4.0f);

        auto version_cbegin = version_string->data();
        auto version_cend = version_cbegin + version_string->size();
        auto version_size = ImGui::CalcTextSize(version_cbegin, version_cend);

        ImVec2 version_pos {};
        version_pos.x = stats_pos.x - version_size.x - padding.x - 0.5f * spacing.x;
        version_pos.y = stats_pos.y + 0.5f * (stats_size.y - version_size.y);

        ImVec2 version_end {};
        version_end.x = version_pos.x + version_size.x;
        version_end.y = version_pos.y + version_size.y;

        ImVec2 outline_pos {};
        outline_pos.x = version_pos.x - 1.0f * static_cast<float>(globals::gui_scale);
        outline_pos.y = std::max(cursor_pos.y, version_pos.y - 1.0f * static_cast<float>(globals::gui_scale));

        ImVec2 outline_end {};
        outline_end.x = version_end.x + 1.0f * static_cast<float>(globals::gui_scale);
        outline_end.y = version_end.y + 1.0f * static_cast<float>(globals::gui_scale);

        auto outline_thickness = std::max(1.0f, 0.5f * static_cast<float>(globals::gui_scale));

        draw_list->AddRect(outline_pos, outline_end, version_color, 0.0f, 0, outline_thickness);
        draw_list->AddText(version_pos, version_color, version_cbegin, version_cend);

        ImGui::PopFont();
    }

    auto motd_shadow_color = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.750f));
    auto motd_shadow_offset = gui::detail::text_shadow_offset();

    for(std::size_t i = 0; i < motd_lines.size(); ++i) {
        auto& line = motd_lines[i];

        ImVec2 motd_pos {};
        motd_pos.x = cursor_pos.x + padding.x + 0.5f * spacing.x;
        motd_pos.y = cursor_pos.y + padding.y + line_height * static_cast<float>(1 + i);

        auto motd_shadow_pos = ImVec2(motd_pos.x + motd_shadow_offset, motd_pos.y + motd_shadow_offset);

        draw_list->AddText(motd_shadow_pos, motd_shadow_color, line.c_str(), line.c_str() + line.size());
        draw_list->AddText(motd_pos, motd_color, line.c_str(), line.c_str() + line.size());
    }
}

void ServerListItem::invalidate(void)
{
    stats_string.reset();
    version_string.reset();
}

void play_menu::init(void)
{
    s_next_identity = 0;

    screen.add_child(s_background, 0);
    screen.add_child(s_body, 2);

    s_body.set_margin(ImVec2(BODY_MARGIN, BODY_MARGIN));
    s_body.enable_background();

    s_worlds_todo.set_text("play_menu.worlds.todo");
    s_worlds_tab.add_child(s_worlds_todo, 0);

    s_btn_join.set_text("play_menu.servers.join");
    s_btn_join.set_size(-1.0f, -1.0f);
    s_btn_join.set_enabled(false);
    s_btn_join.on_click([] {
        if(s_selected_server) {
            join_server(s_selected_server);
        }
    });

    s_btn_connect.add_item("play_menu.servers.connect", [] {
        s_direct_popup.open();
    });

    s_btn_add.set_icon(gui::icon_type::PLUS);
    s_btn_add.set_size(-1.0f, -1.0f);
    s_btn_add.on_click(&open_add_popup);

    s_btn_edit.set_icon(gui::icon_type::PENCIL);
    s_btn_edit.set_size(-1.0f, -1.0f);
    s_btn_edit.set_enabled(false);
    s_btn_edit.on_click(&open_edit_popup);

    s_btn_remove.set_icon(gui::icon_type::MINUS);
    s_btn_remove.set_size(-1.0f, -1.0f);
    s_btn_remove.set_enabled(false);
    s_btn_remove.on_click([] {
        if(s_selected_server) {
            destroy_server(s_selected_server);
        }
    });

    s_btn_refresh.set_icon(gui::icon_type::REFRESH);
    s_btn_refresh.set_size(-1.0f, -1.0f);
    s_btn_refresh.on_click(&refresh_servers);

    s_servers_action_row.add_item(s_btn_add, gui::FIXED, ROW_HEIGHT);
    s_servers_action_row.add_item(s_btn_remove, gui::FIXED, ROW_HEIGHT);
    s_servers_action_row.add_item(s_btn_edit, gui::FIXED, ROW_HEIGHT);
    s_servers_action_row.add_item(s_btn_refresh, gui::FIXED, ROW_HEIGHT);
    s_servers_action_row.add_item(s_btn_join, gui::EXPANDING);
    s_servers_action_row.add_item(s_btn_connect, gui::FIXED, ROW_HEIGHT);

    s_servers_tab.add_item(s_servers_action_row, gui::FIXED, ROW_HEIGHT);
    s_servers_tab.add_item(s_server_list, gui::EXPANDING);

    s_rail.add_page("play_menu.tab.worlds", s_worlds_tab, 0);
    s_rail.add_page("play_menu.tab.servers", s_servers_tab, 1);
    s_rail.set_callback([] {
        main_menu::show();
    });

    s_body.add_child(s_rail);

    s_server_popup.set_title("play_menu.servers.popup.title");
    s_server_popup.add_input("play_menu.servers.popup.name", ImGuiInputTextFlags_None);
    s_server_popup.add_input("play_menu.servers.popup.hostname", ImGuiInputTextFlags_CharsNoBlank);
    s_server_popup.add_input("play_menu.servers.popup.invite", ImGuiInputTextFlags_CharsDecimal);
    screen.add_child(s_server_popup, 3);

    s_direct_popup.set_title("play_menu.servers.direct.title");
    s_direct_popup.add_input("play_menu.servers.direct.hostname", ImGuiInputTextFlags_CharsNoBlank);
    s_direct_popup.add_input("play_menu.servers.direct.invite", ImGuiInputTextFlags_CharsDecimal);
    screen.add_child(s_direct_popup, 3);

    s_direct_popup.on_submit([](std::span<const std::string> values) {
        auto parts = parse_hostname(values[0]);

        std::uint64_t invite = 0;
        auto invite_str = values[1];
        auto invite_size = invite_str.size();

        if(invite_size) {
            std::from_chars(invite_str.data(), invite_str.data() + invite_size, invite);
        }

        session::connect(parts.first, parts.second, invite);
    });

    s_error_popup.set_title("play_menu.connect.error.title");
    s_error_popup.add_choice("play_menu.connect.error.ok");
    screen.add_child(s_error_popup, 3);

    globals::dispatcher.sink<LanguageUpdateEvent>().connect<&on_language_update>();
    globals::dispatcher.sink<SDL_KeyboardEvent>().connect<&on_keyboard_event>();
    globals::dispatcher.sink<BotherResponseEvent>().connect<&on_bother_response>();
    globals::dispatcher.sink<SessionStateEvent>().connect<&on_session_state>();
    globals::dispatcher.sink<SessionErrorEvent>().connect<&on_session_error>();

    load_servers_json();
}

void play_menu::shutdown(void)
{
    save_servers_json();

    s_servers.clear();
}

void play_menu::update_late(void)
{
    for(auto& owned : s_servers) {
        if(owned->configured && owned->status == ServerListItem::UNKNOWN) {
            bother::ping(owned->identity, owned->host, owned->port);
            owned->status = ServerListItem::PINGING;
            owned->invalidate();
        }
    }
}

void play_menu::show_error(std::string_view message)
{
    globals::gui_screen = &screen;

    s_error_popup.set_message(message);
    s_error_popup.open();
}
