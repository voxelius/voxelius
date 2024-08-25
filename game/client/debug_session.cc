// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <FastNoiseLite.h>
#include <game/client/debug_session.hh>
#include <game/client/event/glfw_mouse_button.hh>
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
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
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
