// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/debug_keys.hh>
#include <client/globals.hh>
#include <client/lang.hh>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

bool debug_keys::is_sequence_await = false;

bool debug_keys::draw_bounding_boxes = false;
bool debug_keys::draw_chunk_lines = false;
bool debug_keys::draw_raycasts = false;
bool debug_keys::render_wireframe = false;

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::ui_keybind_ptr) {
        // The UI keybind subsystem has the authority
        // over debug toggles and it hogs the input keys
        return;
    }

    if(event.key == GLFW_KEY_F4) {
        if(event.action == GLFW_PRESS) {
            debug_keys::is_sequence_await = true;
            return;
        }

        if(event.action == GLFW_RELEASE) {
            debug_keys::is_sequence_await = false;
            return;
        }
    }

    if((event.action == GLFW_PRESS) && debug_keys::is_sequence_await) {
        switch(event.key) {
            case GLFW_KEY_B:
                debug_keys::draw_bounding_boxes = !debug_keys::draw_bounding_boxes;
                return;
            case GLFW_KEY_G:
                debug_keys::draw_chunk_lines = !debug_keys::draw_chunk_lines;
                return;
            case GLFW_KEY_J:
                debug_keys::draw_raycasts = !debug_keys::draw_raycasts;
                return;
            case GLFW_KEY_Z:
                debug_keys::render_wireframe = !debug_keys::render_wireframe;
                return;
            case GLFW_KEY_L:
                // This causes the language subsystem
                // to re-parse the JSON file essentially
                // causing the game to soft-reload language
                lang::set(lang::current());
                return;
        }
    }
}

void debug_keys::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}
