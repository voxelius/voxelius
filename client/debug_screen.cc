// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/camera.hh>
#include <client/debug_screen.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;

static bool debug_screen_enable = false;

static std::string gl_version = {};
static std::string gl_renderer = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(globals::ui_keybind_ptr) {
        // The UI keybind subsystem has the authority
        // over debug screen and it hogs the input keys
        return;
    }

    if((event.key == GLFW_KEY_F3) && (event.action == GLFW_PRESS)) {
        debug_screen_enable = !debug_screen_enable;
        return;
    }
}

void debug_screen::init(void)
{
    gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void debug_screen::layout(void)
{
    if(debug_screen_enable) {
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