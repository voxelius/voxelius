// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug_session.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/gui_screen.hh>
#include <client/message_box.hh>
#include <client/player_target.hh>
#include <client/progress.hh>
#include <client/session.hh>
#include <client/voxel_atlas.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <FastNoiseLite.h>
#include <shared/entity/head.hh>
#include <shared/entity/health.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/ray_dda.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>
#include <shared/protocol.hh>

#include <random>

static Voxel v_slate = {};
static Voxel v_stone = {};
static Voxel v_grass = {};
static Voxel v_dirt = {};
static Voxel v_test = {};

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if((event.action == GLFW_PRESS) && (player_target::voxel != NULL_VOXEL)) {
            if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }
            
            if(event.button == GLFW_MOUSE_BUTTON_RIGHT) {
                world::set_voxel(v_stone, player_target::vvec + player_target::vnormal);
                return;
            }
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
}

void debug_session::update(void)
{
    //progress_bar::set_progress(0.5f + 0.5f * std::sin(glfwGetTime()));
}

void debug_session::run(void)
{
    if(globals::session_peer) {
        // Ain't gonna run this twice
        return;
    }

    session::connect("localhost", protocol::PORT);

/*
    if(globals::registry.valid(globals::player)) {
        // Ain't gonna run this twice
        return;
    }

    v_slate = vdef::create("slate", VoxelType::Cube).add_default_state().build();
    v_stone = vdef::create("stone", VoxelType::Cube).add_default_state().build();
    v_grass = vdef::create("grass", VoxelType::Cube).add_default_state().build();
    v_dirt = vdef::create("dirt", VoxelType::Cube).add_default_state().build();
    v_test = vdef::create("vtest", VoxelType::Cube).add_default_state().build();

    ENetHost *hp = enet_host_create(nullptr, 1, 1, 0, 0);
    
    if(!hp) {
        spdlog::critical("no can create host");
        std::terminate();
    }

    ENetAddress address = {};
    address.port = protocol::DEFAULT_PORT;
    enet_address_set_host(&address, "localhost");

    ENetPeer *peer = enet_host_connect(hp, &address, 1, 0);

    if(!peer) {
        spdlog::critical("no can connect");
        std::terminate();
    }
    
    spdlog::warn("connected");

    spdlog::info("spawning local player");
    globals::player = globals::registry.create();
    globals::registry.emplace<PlayerComponent>(globals::player);
    globals::registry.emplace<VelocityComponent>(globals::player);

    auto &head = globals::registry.emplace<HeadComponent>(globals::player);
    auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
    transform.position.local[1] += 32.0;

    progress_bar::reset();
    progress_bar::set_title("Doing something");
    progress_bar::set_button("Stop doing something", [](void) {

        PacketWriter writer = {};
        PacketWriter::string(writer, "Test packet");
        PacketWriter::FP32(writer, 1337.1337f);
        PacketWriter::UI32(writer, 42);

        PacketReader reader = {};
        PacketReader::setup(reader, writer);
        
        const auto x = PacketReader::string(reader);
        const auto y = PacketReader::FP32(reader);
        const auto z = PacketReader::UI32(reader);

        message_box::reset();
        message_box::set_title("Debug");
        message_box::set_subtitle(fmt::format("\"{}\" ({}) {} {}", x, x.size(), y, z));

        message_box::add_button("OK", [](void) {
            // Close the message box and go into the game
            globals::gui_screen = GUI_SCREEN_NONE;
        });

        message_box::add_button("Also OK", [](void) {
            // Close the message box and go into the game
            globals::gui_screen = GUI_SCREEN_NONE;
        });

        globals::gui_screen = GUI_MESSAGE_BOX;
    });

    globals::gui_screen = GUI_PROGRESS_BAR;
*/
}
