#include "client/pch.hh"

#include "client/input/axes.hh"

#include "core/cvar/host.hh"
#include "core/msg/log.hh"

#include "client/globals.hh"
#include "client/input/buttons.hh"

static vx::hash_map<AxisSource, std::string> s_binds {};
static vx::hash_map<std::string, IN_AxisValue> s_slots {};

static void accumulate(input_device device, std::uint32_t code, float value, bool is_delta)
{
    auto it = s_binds.find(AxisSource(device, code));

    if(it == s_binds.end()) {
        return;
    }

    IN_AxisValue axis {};
    axis.is_delta = is_delta;
    axis.value = 0.0f;

    auto inserted = s_slots.try_emplace(std::string(it->second), std::move(axis));
    auto& slot = inserted.first->second;
    slot.is_delta = is_delta;

    if(is_delta) {
        slot.value += value;
    }
    else {
        slot.value = value;
    }
}

template<input_device Device>
static cvar::status cmd_axis(const cvar::Context&, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: {} <axis> [slot]", input::format_axis_cmd(Device));

        return cvar::status::ARGUMENT;
    }

    auto code = input::parse_axis_code(Device, args[0]);

    if(!code.has_value()) {
        auto command = input::format_axis_cmd(Device);
        msg::log_warning("{}: unknown axis {}", command, args[0]);

        return cvar::status::ARGUMENT;
    }

    auto source = AxisSource(Device, code.value());

    if(args.size() < 2) {
        axes::unbind(source);
    }
    else {
        axes::bind(source, std::string(args[1]));
    }

    return cvar::status::OK;
}

static cvar::status cmd_list_axes(const cvar::Context&, std::span<const std::string_view> args)
{
    for(const auto& it : s_binds) {
        auto device = it.first.device();
        auto command = input::format_axis_cmd(device);
        auto code = input::format_axis_code(device, it.first.code());

        msg::log_info("[] {:<8} {:<16} {}", command, code, it.second);
    }

    return cvar::status::OK;
}

static cvar::status cmd_unbind_axes(const cvar::Context&, std::span<const std::string_view>)
{
    axes::clear();

    return cvar::status::OK;
}

static void on_mouse_motion_event(const SDL_MouseMotionEvent& event)
{
    if(buttons::is_capturing || globals::gui_screen) {
        return;
    }

    accumulate(input_device::MOUSE, MAXIS_X, static_cast<float>(event.xrel), true);
    accumulate(input_device::MOUSE, MAXIS_Y, static_cast<float>(event.yrel), true);
}

static void write_config(std::ostream& stream)
{
    std::vector<std::string> lines;
    lines.reserve(s_binds.size());

    for(const auto& it : s_binds) {
        auto device = it.first.device();
        auto command = input::format_axis_cmd(device);
        auto code = input::format_axis_code(device, it.first.code());

        lines.push_back(std::format("{} {} {}", command, code, it.second));
    }

    std::sort(lines.begin(), lines.end());

    for(const auto& line : lines) {
        stream << line;
        stream << std::endl;
    }
}

void axes::init(void)
{
    auto& maxis_binding = cvar::bind_cmd("maxis");
    maxis_binding.set_help("Route a mouse axis into a named slot");
    maxis_binding.connect<&cmd_axis<input_device::MOUSE>>();

    auto& gaxis_binding = cvar::bind_cmd("gaxis");
    gaxis_binding.set_help("Route a gamepad axis into a named slot");
    gaxis_binding.connect<&cmd_axis<input_device::GAMEPAD>>();

    auto& list_axes_binding = cvar::bind_cmd("list_axes");
    list_axes_binding.set_help("List every routed axis");
    list_axes_binding.connect<&cmd_list_axes>();

    auto& unbind_axes_binding = cvar::bind_cmd("unbind_axes");
    unbind_axes_binding.set_help("Remove every routed axis");
    unbind_axes_binding.connect<&cmd_unbind_axes>();

    cvar::add_write_hook<&write_config>();

    globals::dispatcher.sink<SDL_MouseMotionEvent>().connect<&on_mouse_motion_event>();
}

void axes::bind(AxisSource source, std::string slot, bool overwrite)
{
    if(overwrite || 0 == s_binds.count(source)) {
        s_binds.insert_or_assign(source, std::move(slot));
    }
}

void axes::unbind(AxisSource source)
{
    s_binds.erase(source);
}

void axes::clear(void)
{
    s_binds.clear();
    s_slots.clear();
}

IN_AxisValue axes::consume(std::string_view slot)
{
    auto it = s_slots.find(slot);

    if(it == s_slots.end()) {
        IN_AxisValue result {};
        result.is_delta = false;
        result.value = 0.0f;
        return result;
    }

    auto result = it->second;

    if(result.is_delta) {
        it->second.value = 0.0f;
    }

    return result;
}

void axes::reset_all(void)
{
    for(auto& it : s_slots) {
        it.second.value = 0.0f;
    }
}
