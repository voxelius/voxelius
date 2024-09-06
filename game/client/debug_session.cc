// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <FastNoiseLite.h>
#include <game/client/debug_session.hh>
#include <game/client/event/glfw_mouse_button.hh>
#include <game/client/event/glfw_scroll.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/message_box.hh>
#include <game/client/player_target.hh>
#include <game/client/progress.hh>
#include <game/client/session.hh>
#include <game/client/view.hh>
#include <game/client/voxel_atlas.hh>
#include <game/shared/entity/head.hh>
#include <game/shared/entity/player.hh>
#include <game/shared/entity/transform.hh>
#include <game/shared/entity/velocity.hh>
#include <game/shared/game_voxels.hh>
#include <game/shared/protocol.hh>
#include <game/shared/ray_dda.hh>
#include <game/shared/vdef.hh>
#include <game/shared/world.hh>
#include <spdlog/spdlog.h>

static int s_voxnum = 0;
static Voxel s_voxels[5];

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if((event.action == GLFW_PRESS) && (player_target::voxel != NULL_VOXEL)) {
            if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }
            if(event.button == GLFW_MOUSE_BUTTON_RIGHT) {
                world::set_voxel(game_voxels::stone, player_target::vvec + player_target::vnormal);
                return;
            }
            if (event.button == GLFW_MOUSE_BUTTON_MIDDLE) {
                if (++s_voxnum > 4)
                    s_voxnum = 0;
                return;
            }
        }
    }
}

static void on_glfw_scroll(const GlfwScrollEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if(player_target::voxel != NULL_VOXEL) {
            if(event.dx < 0.0) {
                world::set_voxel(s_voxels[s_voxnum], player_target::vvec + player_target::vnormal);
                return;
            }
            if(event.dx > 0.0) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
    globals::dispatcher.sink<GlfwScrollEvent>().connect<&on_glfw_scroll>();
}

void debug_session::update(void)
{

}

void debug_session::run(void)
{
    if(globals::session_peer)
        return;
    session::connect("localhost", protocol::PORT);
}

void debug_session::init_late(void)
{
    s_voxels[0] = game_voxels::dirt;
    s_voxels[1] = game_voxels::grass;
    s_voxels[2] = game_voxels::slate;
    s_voxels[3] = game_voxels::stone;
    s_voxels[4] = game_voxels::vtest;
}
