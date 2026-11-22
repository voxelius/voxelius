#include "client/pch.hh"

#include "client/system/pmove_client.hh"

#include "core/cvar/host.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/system/pmove.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/container.hh"
#include "client/input/axes.hh"
#include "client/input/buttons.hh"
#include "client/settings.hh"

constexpr static std::uint32_t IN_FORWARD = 1 << 0;
constexpr static std::uint32_t IN_BACK = 1 << 1;
constexpr static std::uint32_t IN_LEFT = 1 << 2;
constexpr static std::uint32_t IN_RIGHT = 1 << 3;
constexpr static std::uint32_t IN_JUMP = 1 << 4;
constexpr static std::uint32_t IN_SNEAK = 1 << 5;

static std::uint32_t s_input_held;
static bool s_gui_was_open;

static gui::ButtonBind s_forward_bind;
static gui::ButtonBind s_back_bind;
static gui::ButtonBind s_left_bind;
static gui::ButtonBind s_right_bind;
static gui::ButtonBind s_jump_bind;
static gui::ButtonBind s_sneak_bind;

template<std::uint32_t Bit>
static cvar::status cmd_press(const cvar::Context&, std::span<const std::string_view>)
{
    s_input_held |= Bit;

    return cvar::status::OK;
}

template<std::uint32_t Bit>
static cvar::status cmd_release(const cvar::Context&, std::span<const std::string_view>)
{
    s_input_held &= ~Bit;

    return cvar::status::OK;
}

static void release_all(void)
{
    s_input_held = 0;
    axes::reset_all();
}

static void on_window_event(const SDL_WindowEvent& event)
{
    if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        release_all();
    }
}

void player_move::init(void)
{
    auto& forward_press = cvar::bind_cmd("+forward");
    forward_press.connect<&cmd_press<IN_FORWARD>>();
    auto& forward_release = cvar::bind_cmd("-forward");
    forward_release.connect<&cmd_release<IN_FORWARD>>();
    auto& back_press = cvar::bind_cmd("+back");
    back_press.connect<&cmd_press<IN_BACK>>();
    auto& back_release = cvar::bind_cmd("-back");
    back_release.connect<&cmd_release<IN_BACK>>();
    auto& left_press = cvar::bind_cmd("+moveleft");
    left_press.connect<&cmd_press<IN_LEFT>>();
    auto& left_release = cvar::bind_cmd("-moveleft");
    left_release.connect<&cmd_release<IN_LEFT>>();
    auto& right_press = cvar::bind_cmd("+moveright");
    right_press.connect<&cmd_press<IN_RIGHT>>();
    auto& right_release = cvar::bind_cmd("-moveright");
    right_release.connect<&cmd_release<IN_RIGHT>>();
    auto& jump_press = cvar::bind_cmd("+jump");
    jump_press.connect<&cmd_press<IN_JUMP>>();
    auto& jump_release = cvar::bind_cmd("-jump");
    jump_release.connect<&cmd_release<IN_JUMP>>();
    auto& sneak_press = cvar::bind_cmd("+sneak");
    sneak_press.connect<&cmd_press<IN_SNEAK>>();
    auto& sneak_release = cvar::bind_cmd("-sneak");
    sneak_release.connect<&cmd_release<IN_SNEAK>>();

    s_forward_bind.bind_action("forward");
    s_back_bind.bind_action("back");
    s_left_bind.bind_action("moveleft");
    s_right_bind.bind_action("moveright");
    s_jump_bind.bind_action("jump");
    s_sneak_bind.bind_action("sneak");

    settings::controls_movement.add_child(s_forward_bind, 0);
    settings::controls_movement.add_child(s_back_bind, 1);
    settings::controls_movement.add_child(s_left_bind, 2);
    settings::controls_movement.add_child(s_right_bind, 3);
    settings::controls_movement.add_child(s_jump_bind, 4);
    settings::controls_movement.add_child(s_sneak_bind, 5);

    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_W, input_edge::DOWN), "+forward");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_W, input_edge::UP), "-forward");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_S, input_edge::DOWN), "+back");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_S, input_edge::UP), "-back");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_A, input_edge::DOWN), "+moveleft");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_A, input_edge::UP), "-moveleft");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_D, input_edge::DOWN), "+moveright");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_D, input_edge::UP), "-moveright");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_SPACE, input_edge::DOWN), "+jump");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_SPACE, input_edge::UP), "-jump");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_LSHIFT, input_edge::DOWN), "+sneak");
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_LSHIFT, input_edge::UP), "-sneak");

    globals::dispatcher.sink<SDL_WindowEvent>().connect<&on_window_event>();
}

void player_move::fixed_update(void)
{
    auto& registry = globals::world->registry();

    if(registry.valid(globals::player)) {
        const auto& transform = registry.get<Transform>(globals::player);
        const auto& velocity = registry.get<Velocity>(globals::player);
        registry.emplace_or_replace<Transform_Prev>(globals::player, transform);
        registry.emplace_or_replace<Velocity_Prev>(globals::player, velocity);
    }
}

void player_move::update(void)
{
    auto& registry = globals::world->registry();

    if(globals::gui_screen && !s_gui_was_open) {
        release_all();
    }

    s_gui_was_open = globals::gui_screen;

    if(!registry.valid(globals::player)) {
        return;
    }

    Eigen::Vector3f wishdir = Eigen::Vector3f::Zero();

    if(globals::gui_screen == nullptr) {
        if(s_input_held & IN_FORWARD) {
            wishdir.z() -= 1.0f;
        }

        if(s_input_held & IN_BACK) {
            wishdir.z() += 1.0f;
        }

        if(s_input_held & IN_LEFT) {
            wishdir.x() -= 1.0f;
        }

        if(s_input_held & IN_RIGHT) {
            wishdir.x() += 1.0f;
        }

        if(s_input_held & IN_JUMP) {
            wishdir.y() += 1.0f;
        }

        if(s_input_held & IN_SNEAK) {
            wishdir.y() -= 1.0f;
        }

        if(!wishdir.isZero()) {
            const auto& head = registry.get<Head>(globals::player);
            auto yaw = Eigen::AngleAxisf(head.angles.y(), Eigen::Vector3f::UnitY());
            wishdir = yaw * wishdir;
            wishdir.normalize();
        }
    }

    MoveData move_data {};
    move_data.wishdir = wishdir;

    registry.emplace_or_replace<MoveData>(globals::player, std::move(move_data));
}
