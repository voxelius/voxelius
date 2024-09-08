// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/client/debug_toggles.hh>
#include <game/client/debug_window.hh>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/language.hh>
#include <spdlog/spdlog.h>

bool debug_toggles::is_sequence_await = false;

bool debug_toggles::draw_chunk_borders = false;
bool debug_toggles::render_fullbright = false;
bool debug_toggles::render_wireframe = false;

#if defined(NDEBUG)
bool debug_toggles::draw_debug_screen = false;
#else
bool debug_toggles::draw_debug_screen = true;
#endif

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::gui_keybind_ptr) {
        // The UI keybind subsystem has the authority
        // over debug toggles and it hogs the input keys
        return;
    }

    if(event.key == GLFW_KEY_F3) {
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
            case GLFW_KEY_G:
                debug_toggles::draw_chunk_borders = !debug_toggles::draw_chunk_borders;
                return;
            case GLFW_KEY_V:
                debug_toggles::draw_debug_screen = !debug_toggles::draw_debug_screen;
                return;
            case GLFW_KEY_J:
                debug_toggles::render_fullbright = !debug_toggles::render_fullbright;
                return;
            case GLFW_KEY_Z:
                debug_toggles::render_wireframe = !debug_toggles::render_wireframe;
                return;
            case GLFW_KEY_E:
                debug_window::toggle();
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
