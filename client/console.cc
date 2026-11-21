#include "client/pch.hh"

#include "client/console.hh"

#include "core/cvar/boolean.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/msg/sink.hh"

#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/gui/background.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/command_line.hh"
#include "client/gui/dimmer.hh"
#include "client/gui/log_view.hh"
#include "client/gui/panel.hh"
#include "client/gui/screen.hh"
#include "client/gui/stack.hh"
#include "client/language.hh"
#include "client/main_menu.hh"

constexpr static SDL_Keycode CONSOLE_KEY = SDLK_GRAVE;

constexpr static float SCREEN_HEIGHT = 1.0f;
constexpr static float INPUT_HEIGHT = 14.0f;
constexpr static float CONSOLE_FONT_SIZE = 8.0f;
constexpr static ImVec2 PANEL_MARGIN = ImVec2(1.0f, 1.0f);
constexpr static ImVec4 DIMMER_COLOR = ImVec4(0.000f, 0.000f, 0.000f, 0.750f);

gui::Screen console::screen;

static gui::Dimmer s_dimmer;
static gui::Panel s_panel;
static gui::VerticalStack s_stack;
static gui::LogView s_log_view;
static gui::CommandLine s_input;

static cvar::Number<unsigned> con_history { 1024, 16, 8192 };
static cvar::Boolean con_filter_debug { true };
static cvar::Boolean con_filter_info { true };
static cvar::Boolean con_filter_warning { true };
static cvar::Boolean con_filter_error { true };
static cvar::Boolean con_filter_critical { true };

static std::atomic_bool s_filter_debug { true };
static std::atomic_bool s_filter_info { true };
static std::atomic_bool s_filter_warning { true };
static std::atomic_bool s_filter_error { true };
static std::atomic_bool s_filter_critical { true };

static gui::Screen* s_previous_screen { nullptr };

static cvar::status cmd_clear(const cvar::Context&, std::span<const std::string_view>)
{
    s_log_view.clear();

    return cvar::status::OK;
}

static void console_sink(const msg::Record& record)
{
    auto allow_level = false;
    allow_level = allow_level || s_filter_debug.load() && record.level == msg::LEVEL_DEBUG;
    allow_level = allow_level || s_filter_info.load() && record.level == msg::LEVEL_INFO;
    allow_level = allow_level || s_filter_warning.load() && record.level == msg::LEVEL_WARNING;
    allow_level = allow_level || s_filter_error.load() && record.level == msg::LEVEL_ERROR;
    allow_level = allow_level || s_filter_critical.load() && record.level == msg::LEVEL_CRITICAL;

    if(allow_level) {
        s_log_view.push(record.level, std::string(record.message));
    }
}

static void update_history_size(void)
{
    s_log_view.set_capacity(static_cast<std::size_t>(con_history.value()));
}

static void update_filters(void)
{
    s_filter_debug.store(con_filter_debug.value());
    s_filter_info.store(con_filter_info.value());
    s_filter_warning.store(con_filter_warning.value());
    s_filter_error.store(con_filter_error.value());
    s_filter_critical.store(con_filter_critical.value());
}

static void open_console(void)
{
    s_previous_screen = globals::gui_screen;

    globals::gui_screen = &console::screen;

    s_input.focus();
}

static void close_console(void)
{
    globals::gui_screen = s_previous_screen;

    s_previous_screen = nullptr;

    s_input.clear();
}

static void on_language_update(const LanguageUpdateEvent&)
{
    console::screen.translate();
}

static void on_keyboard_event(const SDL_KeyboardEvent& event)
{
    if(event.type == SDL_EVENT_KEY_DOWN && !event.repeat && gui::ButtonBind::current == nullptr) {
        auto is_open = globals::gui_screen == &console::screen;

        if(event.key == CONSOLE_KEY) {
            if(is_open) {
                close_console();
            }
            else {
                open_console();
            }
        }
        else if(is_open && event.key == SDLK_ESCAPE) {
            close_console();
        }
    }
}

void console::init(void)
{
    msg::add_sink<&console_sink>();

    auto& con_history_binding = cvar::bind_var(con_history, "con_history");
    con_history_binding.set_flags(FCVAR_ARCHIVE);
    con_history_binding.set_help("History size, in messages");
    auto history_changed = con_history_binding.changed();
    history_changed.connect<&update_history_size>();

    auto& con_filter_debug_binding = cvar::bind_var(con_filter_debug, "con_filter_debug");
    con_filter_debug_binding.set_help("Show debug messages in the console");
    con_filter_debug_binding.set_flags(FCVAR_ARCHIVE);
    auto debug_changed = con_filter_debug_binding.changed();
    debug_changed.connect<&update_filters>();

    auto& con_filter_info_binding = cvar::bind_var(con_filter_info, "con_filter_info");
    con_filter_info_binding.set_help("Show info messages in the console");
    con_filter_info_binding.set_flags(FCVAR_ARCHIVE);
    auto info_changed = con_filter_info_binding.changed();
    info_changed.connect<&update_filters>();

    auto& con_filter_warning_binding = cvar::bind_var(con_filter_warning, "con_filter_warning");
    con_filter_warning_binding.set_help("Show warning messages in the console");
    con_filter_warning_binding.set_flags(FCVAR_ARCHIVE);
    auto warning_changed = con_filter_warning_binding.changed();
    warning_changed.connect<&update_filters>();

    auto& con_filter_error_binding = cvar::bind_var(con_filter_error, "con_filter_error");
    con_filter_error_binding.set_help("Show error messages in the console");
    con_filter_error_binding.set_flags(FCVAR_ARCHIVE);
    auto error_changed = con_filter_error_binding.changed();
    error_changed.connect<&update_filters>();

    auto& con_filter_critical_binding = cvar::bind_var(con_filter_critical, "con_filter_critical");
    con_filter_critical_binding.set_help("Show critical messages in the console");
    con_filter_critical_binding.set_flags(FCVAR_ARCHIVE);
    auto critical_changed = con_filter_critical_binding.changed();
    critical_changed.connect<&update_filters>();

    auto& clear_binding = cvar::bind_cmd("clear");
    clear_binding.set_help("Clear the console");
    clear_binding.connect<&cmd_clear>();

    update_history_size();
    update_filters();

    s_input.set_button_text("console.button.submit");
    s_input.on_submit([](std::string_view line) {
        s_log_view.push_echo(std::format("] {}", line));
        cvar::exec(line);
    });

    s_stack.add_item(s_log_view, gui::EXPANDING);
    s_stack.add_item(s_input, gui::FIXED, INPUT_HEIGHT);

    s_panel.set_margin(PANEL_MARGIN);
    s_panel.add_child(s_stack);

    s_dimmer.set_color(DIMMER_COLOR);

    screen.set_start(0.0f, 0.0f);
    screen.set_size(1.0f, SCREEN_HEIGHT);
    screen.add_child(main_menu::background, 0);
    screen.add_child(s_dimmer, 1);
    screen.add_child(s_panel, 2);

    auto language_sink = globals::dispatcher.sink<LanguageUpdateEvent>();
    language_sink.connect<&on_language_update>();

    auto keyboard_sink = globals::dispatcher.sink<SDL_KeyboardEvent>();
    keyboard_sink.connect<&on_keyboard_event>();
}

void console::init_late(void)
{
    s_log_view.set_font(fonts::unscii16, CONSOLE_FONT_SIZE);
    s_input.set_font(fonts::unscii16, CONSOLE_FONT_SIZE);
}

void console::shutdown(void)
{
    msg::remove_sink<&console_sink>();
}
