// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug/debug_toggles.hh>
#include <client/event/glfw_key.hh>
#include <client/gui/language.hh>
#include <client/globals.hh>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

bool debug_toggles::is_sequence_await = false;

bool debug_toggles::draw_bounding_boxes = false;
bool debug_toggles::draw_chunk_lines = false;
bool debug_toggles::draw_raycasts = false;
bool debug_toggles::render_wireframe = false;

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::gui_keybind_ptr) {
        // The UI keybind subsystem has the authority
        // over debug toggles and it hogs the input keys
        return;
    }

    if(event.key == GLFW_KEY_F4) {
        if(event.action == GLFW_PRESS) {
            debug_toggles::is_sequence_await = true;
            return;
        }

        if(event.action == GLFW_RELEASE) {
            debug_toggles::is_sequence_await = false;
            return;
        }
    }

    if((event.action == GLFW_PRESS) && debug_toggles::is_sequence_await) {
        switch(event.key) {
            case GLFW_KEY_B:
                debug_toggles::draw_bounding_boxes = !debug_toggles::draw_bounding_boxes;
                return;
            case GLFW_KEY_G:
                debug_toggles::draw_chunk_lines = !debug_toggles::draw_chunk_lines;
                return;
            case GLFW_KEY_J:
                debug_toggles::draw_raycasts = !debug_toggles::draw_raycasts;
                return;
            case GLFW_KEY_Z:
                debug_toggles::render_wireframe = !debug_toggles::render_wireframe;
                return;
            case GLFW_KEY_L:
                // This causes the language subsystem
                // to re-parse the JSON file essentially
                // causing the game to soft-reload language
                language::set(language::current());
                return;
        }
    }
}

void debug_toggles::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}
