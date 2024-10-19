// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/language.hh>
#include <game/client/toggles.hh>
#include <spdlog/spdlog.h>

bool toggles::is_sequence_await = false;

bool toggles::draw_chunk_borders = false;
bool toggles::render_fullbright = false;
bool toggles::render_wireframe = false;

#if defined(NDEBUG)
bool toggles::draw_metrics = false;
#else
bool toggles::draw_metrics = true;
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
            toggles::is_sequence_await = true;
            return;
        }

        if(event.action == GLFW_RELEASE) {
            toggles::is_sequence_await = false;
            return;
        }
    }

    if((event.action == GLFW_PRESS) && toggles::is_sequence_await) {
        switch(event.key) {
            case GLFW_KEY_G:
                toggles::draw_chunk_borders = !toggles::draw_chunk_borders;
                return;
            case GLFW_KEY_V:
                toggles::draw_metrics = !toggles::draw_metrics;
                return;
            case GLFW_KEY_J:
                toggles::render_fullbright = !toggles::render_fullbright;
                return;
            case GLFW_KEY_Z:
                toggles::render_wireframe = !toggles::render_wireframe;
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

void toggles::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}
