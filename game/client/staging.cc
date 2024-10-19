// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_mouse_button.hh>
#include <game/client/event/glfw_scroll.hh>
#include <game/client/chat.hh>
#include <game/client/globals.hh>
#include <game/client/player_target.hh>
#include <game/client/staging.hh>
#include <game/shared/game_voxels.hh>
#include <game/shared/world.hh>
#include <spdlog/fmt/fmt.h>
#include <vector>

static std::size_t place_index = 0;
static std::vector<Voxel> place_voxels = {};

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        if((event.action == GLFW_PRESS) && (player_target::voxel != NULL_VOXEL)) {
            if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
                world::set_voxel(NULL_VOXEL, player_target::vvec);
                return;
            }

            if(event.button == GLFW_MOUSE_BUTTON_RIGHT) {
                world::set_voxel(place_voxels[place_index], player_target::vvec + player_target::vnormal);
                return;
            }
        }
    }
}

static void on_glfw_scroll(const GlfwScrollEvent &event)
{
    if(const auto delta = cxpr::sign<int>(event.dy)) {
        if((delta > 0) && (place_index > 0))
            place_index -= 1;
        if((delta < 0) && (place_index < (place_voxels.size() - 1)))
            place_index += 1;
        const auto info = vdef::find(place_voxels[place_index]);
        client_chat::print(fmt::format("[debug] {}", info ? info->name : std::string("nullptr")));
    }
}

void staging::init(void)
{
    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
    globals::dispatcher.sink<GlfwScrollEvent>().connect<&on_glfw_scroll>();
}

void staging::init_late(void)
{
    place_voxels.push_back(game_voxels::dirt);
    place_voxels.push_back(game_voxels::grass);
    place_voxels.push_back(game_voxels::slate);
    place_voxels.push_back(game_voxels::stone);
    place_voxels.push_back(game_voxels::vtest);
}

void staging::deinit(void)
{

}

void staging::update(void)
{

}

void staging::update_late(void)
{

}
