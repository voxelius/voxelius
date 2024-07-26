// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/camera.hh>
#include <client/debug.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/lang.hh>
#include <client/settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <shared/cmake.hh>
#include <shared/config.hh>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;

bool debug::active_sequence = false;
bool debug::render_wireframe = false;

static int key_toggle = GLFW_KEY_F3;
static int key_sequence = GLFW_KEY_F4;
static int key_wireframe = GLFW_KEY_Z;
static int key_language = GLFW_KEY_L;

static bool debug_gui_enable = false;

static std::string gl_version = {};
static std::string gl_renderer = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::ui_keybind_ptr) {
        // The setting menu requested a
        // key to be pressed; we must not
        // interfere with it's businiess
        return;
    }

    if(event.action == GLFW_PRESS) {
        if(event.key == key_toggle) {
            debug_gui_enable = !debug_gui_enable;
            return;
        }
        
        if(event.key == key_sequence) {
            debug::active_sequence = true;
            return;
        }
        
        if(debug::active_sequence) {
            if(event.key == key_wireframe) {
                debug::render_wireframe = !debug::render_wireframe;
                return;
            }
            
            if(event.key == key_language) {
                lang::set(lang::current());
                return;
            }
        }
    }

    if(event.action == GLFW_RELEASE) {
        if(event.key == key_sequence) {
            debug::active_sequence = false;
            return;
        }
    }
}

void debug::init(void)
{
    Config::add(globals::client_config, "key.debug.toggle", key_toggle);
    Config::add(globals::client_config, "key.debug.sequence", key_sequence);
    Config::add(globals::client_config, "key.debug.wireframe", key_wireframe);
    Config::add(globals::client_config, "key.debug.language", key_language);

    settings::add_key_binding(1, settings::KEYBOARD_MISC, "key.debug.toggle", key_toggle);
    settings::add_key_binding(2, settings::KEYBOARD_MISC, "key.debug.sequence", key_sequence);
    settings::add_key_binding(3, settings::KEYBOARD_MISC, "key.debug.wireframe", key_wireframe);
    settings::add_key_binding(3, settings::KEYBOARD_MISC, "key.debug.language", key_language);

    gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void debug::layout(void)
{
    if(debug_gui_enable) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::PushFont(globals::font_debug);

        if(ImGui::Begin("###debug", nullptr, MENU_FLAGS)) {
            const float ui_framerate = 1.0f / globals::frametime_avg;
            const float ui_frametime = 1000.0f * globals::frametime_avg;
  
            if(client_game::vertical_sync)
                ImGui::Text("%.02f FPS [%.02f ms] [VSYNC]", ui_framerate, ui_frametime);
            else ImGui::Text("%.02f FPS [%.02f ms]", ui_framerate, ui_frametime);

            ImGui::Text("World triangles: %zu", globals::num_triangles);
            ImGui::Text("GL_VERSION: %s", gl_version.c_str());
            ImGui::Text("GL_RENDERER: %s", gl_renderer.c_str());
            
            const EntityPos &pos = camera::position();
            const auto cam_vpos = glm::vec<3, std::intmax_t>(coord::to_voxel(pos));
            const auto cam_cpos = glm::vec<3, std::intmax_t>(pos.chunk);
            ImGui::Text("voxel: [%jd; %jd; %jd]", cam_vpos.x, cam_vpos.y, cam_vpos.z);
            ImGui::Text("chunk: [%jd; %jd; %jd]", cam_cpos.x, cam_cpos.y, cam_cpos.z);
        }
        
        ImGui::PopFont();
        ImGui::End();
    }
    
}
