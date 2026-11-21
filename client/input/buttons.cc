#include "client/pch.hh"

#include "client/input/buttons.hh"

#include "core/cvar/host.hh"
#include "core/msg/log.hh"
#include "core/utils/string.hh"

#include "client/globals.hh"

bool buttons::is_capturing = false;

static vx::hash_map<ButtonSource, std::string> s_binds {};

static std::string join_command(std::span<const std::string_view> args)
{
    std::vector<std::string> parts;
    parts.reserve(args.size());

    for(auto& it : args) {
        if(utils::has_whitespace(it)) {
            parts.push_back(std::format("\"{}\"", utils::escape_quotations(it)));
        }
        else {
            parts.push_back(std::string(it));
        }
    }

    return utils::join(parts, " ");
}

template<input_device Device>
static cvar::status cmd_button(const cvar::Context&, std::span<const std::string_view> args)
{
    if(args.size() < 2) {
        msg::log_warning("usage: {} <button> <down|up> [command...]", input::format_button_cmd(Device));

        return cvar::status::ARGUMENT;
    }

    auto code = input::parse_button_code(Device, args[0]);

    if(!code.has_value()) {
        auto command = input::format_button_cmd(Device);
        msg::log_warning("{}: unknown button {}", command, args[0]);

        return cvar::status::ARGUMENT;
    }

    auto edge = input::parse_edge(args[1]);

    if(!edge.has_value()) {
        auto command = input::format_button_cmd(Device);
        msg::log_warning("{}: expected down or up, got {}", command, args[1]);

        return cvar::status::ARGUMENT;
    }

    auto source = ButtonSource(Device, code.value(), edge.value());
    auto command = join_command(args.subspan(2));

    if(command.empty()) {
        buttons::unbind(source);
    }
    else {
        buttons::bind(source, std::move(command));
    }

    return cvar::status::OK;
}

static cvar::status cmd_list_buttons(const cvar::Context&, std::span<const std::string_view> args)
{
    for(const auto& it : s_binds) {
        auto device = it.first.device();
        auto command = input::format_button_cmd(device);
        auto code = input::format_button_code(device, it.first.code());
        auto edge = input::format_edge(it.first.edge());

        msg::log_info("[] {:<8} {:<16} {:<8} {}", command, code, edge, it.second);
    }

    return cvar::status::OK;
}

static cvar::status cmd_unbind_buttons(const cvar::Context&, std::span<const std::string_view>)
{
    buttons::clear();

    return cvar::status::OK;
}

static void execute(input_device device, std::uint32_t code, input_edge edge)
{
    auto& io = ImGui::GetIO();

    if(buttons::is_capturing || globals::gui_screen || io.WantCaptureKeyboard) {
        return;
    }

    auto it = s_binds.find(ButtonSource(device, code, edge));

    if(it == s_binds.cend()) {
        return;
    }

    cvar::exec(it->second);
}

static void on_keyboard_event(const SDL_KeyboardEvent& event)
{
    if(event.repeat) {
        return;
    }

    if(event.type == SDL_EVENT_KEY_DOWN) {
        execute(input_device::KEYBOARD, static_cast<std::uint32_t>(event.scancode), input_edge::DOWN);
    }
    else {
        execute(input_device::KEYBOARD, static_cast<std::uint32_t>(event.scancode), input_edge::UP);
    }
}

static void on_mouse_button_event(const SDL_MouseButtonEvent& event)
{
    if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        execute(input_device::MOUSE, static_cast<std::uint32_t>(event.button), input_edge::DOWN);
    }
    else {
        execute(input_device::MOUSE, static_cast<std::uint32_t>(event.button), input_edge::UP);
    }
}

static void execute_pulse(std::uint32_t code)
{
    execute(input_device::MWHEEL, code, input_edge::DOWN);
    execute(input_device::MWHEEL, code, input_edge::UP);
}

static void on_mouse_wheel_event(const SDL_MouseWheelEvent& event)
{
    if(event.x > 0.0f) {
        execute_pulse(MWHEEL_XUP);
    }
    else if(event.x < 0.0f) {
        execute_pulse(MWHEEL_XDOWN);
    }

    if(event.y > 0.0f) {
        execute_pulse(MWHEEL_YUP);
    }
    else if(event.y < 0.0f) {
        execute_pulse(MWHEEL_YDOWN);
    }
}

static void write_config(std::ostream& stream)
{
    std::vector<std::string> lines;
    lines.reserve(s_binds.size());

    for(const auto& it : s_binds) {
        auto device = it.first.device();
        auto command = input::format_button_cmd(device);
        auto code = input::format_button_code(device, it.first.code());
        auto edge = input::format_edge(it.first.edge());

        lines.push_back(std::format("{} {} {} {}", command, code, edge, it.second));
    }

    std::sort(lines.begin(), lines.end());

    for(const auto& line : lines) {
        stream << line;
        stream << std::endl;
    }
}

void buttons::init(void)
{
    auto& kbutton_binding = cvar::bind_cmd("kbutton");
    kbutton_binding.set_help("Bind a keyboard key edge to a command");
    kbutton_binding.connect<&cmd_button<input_device::KEYBOARD>>();

    auto& mbutton_binding = cvar::bind_cmd("mbutton");
    mbutton_binding.set_help("Bind a mouse button edge to a command");
    mbutton_binding.connect<&cmd_button<input_device::MOUSE>>();

    auto& mwheel_binding = cvar::bind_cmd("mwheel");
    mwheel_binding.set_help("Bind a mouse wheel axis direction to a command");
    mwheel_binding.connect<&cmd_button<input_device::MWHEEL>>();

    auto& gbutton_binding = cvar::bind_cmd("gbutton");
    gbutton_binding.set_help("Bind a gamepad button edge to a command");
    gbutton_binding.connect<&cmd_button<input_device::GAMEPAD>>();

    auto& list_buttons_binding = cvar::bind_cmd("list_buttons");
    list_buttons_binding.set_help("List every bound button");
    list_buttons_binding.connect<&cmd_list_buttons>();

    auto& unbind_buttons_binding = cvar::bind_cmd("unbind_buttons");
    unbind_buttons_binding.set_help("Remove every button bind");
    unbind_buttons_binding.connect<&cmd_unbind_buttons>();

    cvar::add_write_hook<&write_config>();

    globals::dispatcher.sink<SDL_KeyboardEvent>().connect<&on_keyboard_event>();
    globals::dispatcher.sink<SDL_MouseButtonEvent>().connect<&on_mouse_button_event>();
    globals::dispatcher.sink<SDL_MouseWheelEvent>().connect<&on_mouse_wheel_event>();
}

void buttons::bind(ButtonSource source, std::string command, bool overwrite)
{
    if(overwrite || 0 == s_binds.count(source)) {
        s_binds.insert_or_assign(source, std::move(command));
    }
}

void buttons::unbind(ButtonSource source)
{
    s_binds.erase(source);
}

void buttons::clear(void)
{
    s_binds.clear();
}

std::string_view buttons::find_command(ButtonSource source)
{
    auto it = s_binds.find(source);

    if(it == s_binds.end()) {
        return {};
    }
    else {
        return it->second;
    }
}

std::vector<ButtonSource> buttons::find(std::string_view command)
{
    std::vector<ButtonSource> result;

    for(const auto& [source, bound] : s_binds) {
        if(bound == command) {
            result.push_back(source);
        }
    }

    return result;
}
