// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/debug.hh>
#include <client/globals.hh>
#include <client/lang.hh>
#include <client/settings.hh>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <shared/config.hh>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar;

constexpr static unsigned int DEBUG_LINES_NONE  = 0U; // Nothing
constexpr static unsigned int DEBUG_LINES_FPS   = 1U; // FPS and coordinates
constexpr static unsigned int DEBUG_LINES_STATS = 2U; // Performance statistics
constexpr static unsigned int DEBUG_LINES_COUNT = 3U;

bool debug::active_sequence = false;
bool debug::render_wireframe = false;

static int key_line_toggle = GLFW_KEY_F3;
static int key_sequence_start = GLFW_KEY_F4;
static int key_sequence_wireframe = GLFW_KEY_Z;
static int key_sequence_language = GLFW_KEY_L;

static unsigned int debug_lines = DEBUG_LINES_NONE;

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::ui_keybind_ptr) {
        // Binding a key, debug toggles
        // are temporarily disabled
        return;
    }

    if((event.key == key_line_toggle) && (event.action == GLFW_PRESS)) {
        debug_lines += 1U;
        return;
    }

    if(event.key == key_sequence_start) {
        if(event.action == GLFW_PRESS) {
            debug::active_sequence = true;
            return;
        }
        
        if(event.action == GLFW_RELEASE) {
            debug::active_sequence = false;
            return;
        }
    }

    if(debug::active_sequence && (event.action == GLFW_PRESS)) {
        if(event.key == key_sequence_wireframe) {
            debug::render_wireframe = !debug::render_wireframe;
            return;
        }
        
        if(event.key == key_sequence_language) {
            lang::set(lang::current());
            return;
        }
    }
}

void debug::init(void)
{
    Config::add(globals::client_config, "key.debug.line_toggle", key_line_toggle);
    Config::add(globals::client_config, "key.debug.sequence_start", key_sequence_start);
    Config::add(globals::client_config, "key.debug.sequence_wireframe", key_sequence_wireframe);
    Config::add(globals::client_config, "key.debug.sequence_language", key_sequence_language);
    Config::add(globals::client_config, "debug.lines", debug_lines);
    
    settings::add_key_binding(1, settings::KEYBOARD_MISC, "key.debug.line_toggle", key_line_toggle);
    settings::add_key_binding(2, settings::KEYBOARD_MISC, "key.debug.sequence_start", key_sequence_start);
    settings::add_key_binding(3, settings::KEYBOARD_MISC, "key.debug.sequence_wireframe", key_sequence_wireframe);
    settings::add_key_binding(3, settings::KEYBOARD_MISC, "key.debug.sequence_language", key_sequence_language);
    
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void debug::layout(void)
{
    

}

/*
void debug::layout(void)
{
    debug_draw_level %= DRAW_MAX;

    if(!debug_draw_level) {
        // Debug lines disabled
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

    if(ImGui::Begin("###debug", nullptr, MENU_FLAGS)) {
        ImGui::Text("TEST");
        
    }
    
    ImGui::End();
}
*/
