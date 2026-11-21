#include "client/pch.hh"

#include "client/system/pmove_look.hh"

#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/utils/angles.hh"

#include "shared/component/head.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/input/axes.hh"
#include "client/settings.hh"

constexpr static float PITCH_MIN = -1.0f * utils::radians(90.0f);
constexpr static float PITCH_MAX = +1.0f * utils::radians(90.0f);

static cvar::Number<unsigned> s_sensitivity { 100, 25, 100 };
static cvar::Boolean s_software_cursor { false };
static gui::SliderUnsigned s_sensitivity_gui;
static gui::CheckBox s_software_cursor_gui;

static void add_angles(float pitch, float yaw)
{
    auto& registry = globals::world->registry();

    if(registry.valid(globals::player)) {
        auto& head = registry.get<Head>(globals::player);

        head.angles[0] += pitch;
        head.angles[1] += yaw;
        head.angles[0] = std::clamp(head.angles[0], PITCH_MIN, PITCH_MAX);
        head.angles = utils::wrap_angle_180(head.angles);

        // Client-side head angles are not interpolated; re-assigning the previous state forces it
        registry.emplace_or_replace<Head_Prev>(globals::player, head);
    }
}

void player_look::init(void)
{
    auto& in_sensitivity_binding = cvar::bind_var(s_sensitivity, "in_sensitivity");
    in_sensitivity_binding.set_help("Mouse sensitivity");
    in_sensitivity_binding.set_flags(FCVAR_ARCHIVE);

    auto& sensitivity_range = s_sensitivity_gui.set_range(25, 100);
    sensitivity_range.enable_tooltip();
    s_sensitivity_gui.bind("in_sensitivity");
    settings::mouse.add_child(s_sensitivity_gui, 0);

    auto& r_software_cursor_binding = cvar::bind_var(s_software_cursor, "r_software_cursor");
    r_software_cursor_binding.set_help("Draw the menu cursor in software");
    r_software_cursor_binding.set_flags(FCVAR_ARCHIVE);

    s_software_cursor_gui.enable_tooltip();
    s_software_cursor_gui.bind("r_software_cursor");
    settings::mouse.add_child(s_software_cursor_gui, 1);

    axes::bind(AxisSource(input_device::MOUSE, MAXIS_X), "pm_look_x");
    axes::bind(AxisSource(input_device::MOUSE, MAXIS_Y), "pm_look_y");
}

void player_look::update(void)
{
    auto look_x = axes::consume("pm_look_x");
    auto look_y = axes::consume("pm_look_y");

    if(look_x.value == 0.0f && look_y.value == 0.0f) {
        return;
    }

    auto dx = -0.01f * static_cast<float>(s_sensitivity.value()) * utils::radians(look_x.value);
    auto dy = -0.01f * static_cast<float>(s_sensitivity.value()) * utils::radians(look_y.value);
    add_angles(dy, dx);
}

void player_look::update_late(void)
{
    auto& io = ImGui::GetIO();

    if(globals::gui_screen) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        io.MouseDrawCursor = s_software_cursor.value();
        SDL_SetWindowRelativeMouseMode(globals::window, false);
    }
    else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.MouseDrawCursor = false;
        SDL_SetWindowRelativeMouseMode(globals::window, true);
    }
}
