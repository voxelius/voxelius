// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <FastNoiseLite.h>
#include <game/client/debug_session.hh>
#include <game/client/event/glfw_mouse_button.hh>
#include <game/client/event/glfw_scroll.hh>
#include <game/client/chat.hh>
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

static std::size_t place_voxnum = 0;
static std::vector<Voxel> place_voxels = {};

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if((event.action == GLFW_PRESS) && (player_target::voxel != NULL_VOXEL)) {
            if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }

            if((event.button == GLFW_MOUSE_BUTTON_RIGHT) && (glfwGetKey(globals::window, GLFW_KEY_LEFT_ALT) != GLFW_PRESS)) {
                world::set_voxel(place_voxels[place_voxnum], player_target::vvec + player_target::vnormal);
                return;
            }
        }

        if((event.action == GLFW_PRESS) && (event.button == GLFW_MOUSE_BUTTON_MIDDLE)) {
            place_voxnum += 1;
            place_voxnum %= place_voxels.size();
            if(const VoxelInfo *info = vdef::find(place_voxels[place_voxnum]))
                client_chat::print(fmt::format("[debug] place_voxel: {}", info->name));
            return;
        }
    }
}

void debug_session::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
}

void debug_session::update(void)
{
    if(player_target::voxel != NULL_VOXEL) {
        if(glfwGetKey(globals::window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            if(glfwGetMouseButton(globals::window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                world::set_voxel(place_voxels[place_voxnum], player_target::vvec + player_target::vnormal);
                return;
            }
        }
    }
}

void debug_session::init_late(void)
{
    place_voxels.push_back(game_voxels::dirt);
    place_voxels.push_back(game_voxels::grass);
    place_voxels.push_back(game_voxels::slate);
    place_voxels.push_back(game_voxels::stone);
    place_voxels.push_back(game_voxels::vtest);
}
