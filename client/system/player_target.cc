#include "client/pch.hh"

#include "client/system/player_target.hh"

#include "core/camera.hh"
#include "core/cvar/host.hh"
#include "core/identifier.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/net/packet_player.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/block_collisions.hh"
#include "shared/world/block_registry.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/container.hh"
#include "client/input/buttons.hh"
#include "client/settings.hh"
#include "client/world/outline.hh"

physics::Hit player_target::hit;

static gui::ButtonBind s_break_bind;
static gui::ButtonBind s_place_bind;

static void place_block(void)
{
    if(!gamerules::allow_building.value()) {
        return;
    }

    if(globals::gui_screen || !std::holds_alternative<physics::BlockHit>(player_target::hit)) {
        return;
    }

    const auto& hit = std::get<physics::BlockHit>(player_target::hit);

    // TODO: hardcoded placed block until an inventory/hotbar system exists
    auto emitter_id = Identifier::from_string("builtin:emitter");
    auto family = block_registry::find_family(emitter_id);
    block_id_type block;

    if(family) {
        block = family->default_variant;
    }
    else {
        block = block_registry::find(emitter_id);
    }

    utils::block_place(hit, globals::player, block);

    if(globals::peer) {
        packet::Player_InteractB packet {};
        packet.bpos = hit.block_pos;
        packet.expected = hit.id;
        packet.face = hit.face;
        packet.normal = hit.normal;
        packet.point = hit.point;
        protocol::send(packet, globals::peer);
    }
}

static void break_block(void)
{
    if(!gamerules::allow_building.value()) {
        return;
    }

    if(globals::gui_screen || !std::holds_alternative<physics::BlockHit>(player_target::hit)) {
        return;
    }

    const auto& hit = std::get<physics::BlockHit>(player_target::hit);

    utils::block_break(hit, globals::player);

    if(globals::peer) {
        packet::Player_AttackB packet {};
        packet.bpos = hit.block_pos;
        packet.expected = hit.id;
        protocol::send(packet, globals::peer);
    }
}

static cvar::status cmd_place(const cvar::Context&, std::span<const std::string_view>)
{
    place_block();

    return cvar::status::OK;
}

static cvar::status cmd_break(const cvar::Context&, std::span<const std::string_view>)
{
    break_block();

    return cvar::status::OK;
}

static cvar::status cmd_release(const cvar::Context&, std::span<const std::string_view>)
{
    return cvar::status::OK;
}

void player_target::init(void)
{
    hit = std::monostate();

    auto& place_command = cvar::bind_cmd("+place");
    place_command.connect<&cmd_place>();

    auto& release_place_command = cvar::bind_cmd("-place");
    release_place_command.connect<&cmd_release>();

    auto& break_command = cvar::bind_cmd("+break");
    break_command.connect<&cmd_break>();

    auto& release_break_command = cvar::bind_cmd("-break");
    release_break_command.connect<&cmd_release>();

    buttons::bind(ButtonSource(input_device::MOUSE, SDL_BUTTON_RIGHT, input_edge::DOWN), "+place");
    buttons::bind(ButtonSource(input_device::MOUSE, SDL_BUTTON_RIGHT, input_edge::UP), "-place");
    buttons::bind(ButtonSource(input_device::MOUSE, SDL_BUTTON_LEFT, input_edge::DOWN), "+break");
    buttons::bind(ButtonSource(input_device::MOUSE, SDL_BUTTON_LEFT, input_edge::UP), "-break");

    s_break_bind.bind_action("break");
    s_place_bind.bind_action("place");

    settings::controls_gameplay.add_child(s_break_bind, 0);
    settings::controls_gameplay.add_child(s_place_bind, 1);
}

void player_target::update(void)
{
    physics::Ray ray {};
    ray.world = globals::world;
    ray.start_chunk = client_camera::chunk;
    ray.start = client_camera::instance.position();
    ray.direction = client_camera::forward;
    ray.max_distance = gamerules::reach_distance.value();

    hit = physics::raycast(ray, physics::BLOCK_FILTER_SELECTABLE, physics::ENTITY_FILTER_ALL);
}

void player_target::render(void)
{
    if(auto block_hit = std::get_if<physics::BlockHit>(&hit)) {
        if(auto bcoll = block_collisions::find(block_hit->id)) {
            Eigen::AlignedBox3f aabb = bcoll->bounds;
            aabb.translate(block_hit->local_pos.cast<float>());

            glEnable(GL_DEPTH_TEST);

            outline::prepare();
            outline::cube(block_hit->chunk_pos, aabb.min(), aabb.sizes(), 2.0f, Eigen::Vector4f(0.00f, 0.00f, 0.00f, 1.00f));
        }
    }
}
