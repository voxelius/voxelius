#include "client/pch.hh"

#include "client/gui/button_bind.hh"

#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/res/resource.hh"

#include "shared/constant.hh"

#include "client/globals.hh"
#include "client/gui/stone.hh"
#include "client/input/buttons.hh"
#include "client/res/texture_2d.hh"

gui::ButtonBind* gui::ButtonBind::current = nullptr;

static res::handle<Texture2D> s_icon_kbutton;
static res::handle<Texture2D> s_icon_mbutton;
static res::handle<Texture2D> s_icon_mwheel;
static res::handle<Texture2D> s_icon_gbutton;
static res::handle<Texture2D> s_icon_missing;

static const Texture2D* icon_for(std::optional<input_device> device)
{
    if(device.has_value()) {
        switch(device.value()) {
            case input_device::KEYBOARD:
                return s_icon_kbutton.get();

            case input_device::MOUSE:
                return s_icon_mbutton.get();

            case input_device::MWHEEL:
                return s_icon_mwheel.get();

            case input_device::GAMEPAD:
                return s_icon_gbutton.get();
        }
    }

    return s_icon_missing.get();
}

static res::handle<Texture2D> load_icon(std::string_view name)
{
    auto id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, name);
    auto handle = res::load<Texture2D>(id, "textures/gui", RESFLAG_TEX_NEAREST);

    vx::throw_if_not_fmt(static_cast<bool>(handle), "{}: failed to load bind icon", id.full_string());

    return handle;
}

void gui::ButtonBind::init(void)
{
    s_icon_kbutton = load_icon("bind_kbutton.png");
    s_icon_mbutton = load_icon("bind_mbutton.png");
    s_icon_mwheel = load_icon("bind_mwheel.png");
    s_icon_gbutton = load_icon("bind_gbutton.png");
    s_icon_missing = load_icon("bind_missing.png");

    auto keyboard_sink = globals::dispatcher.sink<SDL_KeyboardEvent>();
    auto mouse_button_sink = globals::dispatcher.sink<SDL_MouseButtonEvent>();
    auto mouse_wheel_sink = globals::dispatcher.sink<SDL_MouseWheelEvent>();

    keyboard_sink.connect<&gui::ButtonBind::on_keyboard_event>();
    mouse_button_sink.connect<&gui::ButtonBind::on_mouse_button_event>();
    mouse_wheel_sink.connect<&gui::ButtonBind::on_mouse_wheel_event>();
}

void gui::ButtonBind::shutdown(void)
{
    s_icon_missing.reset();
    s_icon_gbutton.reset();
    s_icon_mwheel.reset();
    s_icon_mbutton.reset();
    s_icon_kbutton.reset();
}

gui::ButtonBind& gui::ButtonBind::bind_action(std::string_view action, unsigned slot)
{
    set_key(std::format("action.{}", action));

    m_action = action;
    m_press = std::format("+{}", action);
    m_release = std::format("-{}", action);
    m_slot = slot;

    return self();
}

std::optional<ButtonSource> gui::ButtonBind::bound_button(void) const
{
    auto sources = buttons::find(m_press);

    if(m_slot < sources.size()) {
        return sources[m_slot];
    }

    return std::nullopt;
}

void gui::ButtonBind::capture(std::optional<ButtonSource> source)
{
    if(source.has_value()) {
        auto pressed = source.value();
        auto released = ButtonSource(pressed.device(), pressed.code(), input_edge::UP);

        buttons::unbind(pressed);
        buttons::unbind(released);

        if(auto previous = bound_button()) {
            buttons::unbind(previous.value());
            buttons::unbind(ButtonSource(previous->device(), previous->code(), input_edge::UP));
        }

        buttons::bind(pressed, m_press);
        buttons::bind(released, m_release);
    }

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    buttons::is_capturing = false;

    current = nullptr;

    translate_control();
}

void gui::ButtonBind::layout_control(void)
{
    auto label = m_bound_label.c_str();
    auto source = bound_button();
    std::optional<input_device> device = std::nullopt;

    if(source.has_value()) {
        device = source.value().device();
    }

    if(current == this) {
        label = m_capture_label.c_str();
        device = std::nullopt;
    }

    auto control_width = ImGui::CalcItemWidth();
    auto& item_spacing = ImGui::GetStyle().ItemSpacing;

    auto icon = icon_for(device);

    // The textures are drawn at the height of the stone button, so the
    // bind row lines up with the other controls instead of the text
    auto icon_height = ImGui::GetFrameHeight();
    auto icon_width = icon_height * static_cast<float>(icon->width) / static_cast<float>(icon->height);
    auto icon_stride = icon_width + item_spacing.x;

    auto draw_list = ImGui::GetWindowDrawList();
    const auto& platform_io = ImGui::GetPlatformIO();

    auto cursor = ImGui::GetCursorPos();

    ImGui::SetCursorPosY(cursor.y + (ImGui::GetFrameHeight() - icon_height) * 0.5f);

    if(platform_io.DrawCallback_SetSamplerNearest) {
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
    }

    ImGui::Image(icon->imgui, ImVec2(icon_width, icon_height));

    if(platform_io.DrawCallback_SetSamplerLinear) {
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosY(cursor.y);

    if(gui::detail::stone_button(label, label, ImVec2(control_width - icon_stride, ImGui::GetFrameHeight()))) {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        buttons::is_capturing = true;

        current = this;
    }
}

void gui::ButtonBind::translate_control(void)
{
    m_capture_label = std::format("...{}", imgui_id());

    if(auto source = bound_button()) {
        auto name = input::format_button_code(source->device(), source->code());

        m_bound_label = std::format("{}{}", name, imgui_id());
    }
    else {
        m_bound_label = std::format("---{}", imgui_id());
    }
}

void gui::ButtonBind::on_keyboard_event(const SDL_KeyboardEvent& event)
{
    if(current && event.type == SDL_EVENT_KEY_DOWN) {
        if(event.key == SDLK_ESCAPE) {
            current->capture(std::nullopt);
        }
        else {
            auto code = static_cast<std::uint32_t>(event.scancode);

            current->capture(ButtonSource(input_device::KEYBOARD, code, input_edge::DOWN));
        }
    }
}

void gui::ButtonBind::on_mouse_button_event(const SDL_MouseButtonEvent& event)
{
    if(current && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        current->capture(ButtonSource(input_device::MOUSE, static_cast<std::uint32_t>(event.button), input_edge::DOWN));
    }
}

void gui::ButtonBind::on_mouse_wheel_event(const SDL_MouseWheelEvent& event)
{
    if(current == nullptr) {
        return;
    }

    if(event.y > 0.0) {
        current->capture(ButtonSource(input_device::MWHEEL, MWHEEL_YUP, input_edge::DOWN));
    }
    else if(event.y < 0.0) {
        current->capture(ButtonSource(input_device::MWHEEL, MWHEEL_YDOWN, input_edge::DOWN));
    }
    else if(event.x > 0.0) {
        current->capture(ButtonSource(input_device::MWHEEL, MWHEEL_XUP, input_edge::DOWN));
    }
    else if(event.x < 0.0) {
        current->capture(ButtonSource(input_device::MWHEEL, MWHEEL_XDOWN, input_edge::DOWN));
    }
}
