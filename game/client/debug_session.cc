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

static int voxnum = 0;

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
                if (++voxnum > 4)
                    voxnum = 0;
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
                switch (voxnum) {
                    case 0:
                        world::set_voxel(game_voxels::dirt, player_target::vvec + player_target::vnormal);
                        break;
                    case 1:
                        world::set_voxel(game_voxels::grass, player_target::vvec + player_target::vnormal);
                        break;
                    case 2:
                        world::set_voxel(game_voxels::slate, player_target::vvec + player_target::vnormal);
                        break;
                    case 3:
                        world::set_voxel(game_voxels::stone, player_target::vvec + player_target::vnormal);
                        break;
                    case 4:
                        world::set_voxel(game_voxels::vtest, player_target::vvec + player_target::vnormal);
                        break;
                }
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
