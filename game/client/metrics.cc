// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/game.hh>
#include <game/client/globals.hh>
#include <game/client/metrics.hh>
#include <game/client/view.hh>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;

static std::string gl_version = {};
static std::string gl_renderer = {};

void metrics::init(void)
{
    gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
}

void metrics::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::PushFont(globals::font_debug);

    if(ImGui::Begin("###metrics", nullptr, WINDOW_FLAGS)) {
        const float ui_framerate = 1.0f / globals::frametime_avg;
        const float ui_frametime = 1000.0f * globals::frametime_avg;
  
        if(client_game::vertical_sync)
            ImGui::Text("%.02f FPS [%.02f ms] [VSYNC]", ui_framerate, ui_frametime);
        else ImGui::Text("%.02f FPS [%.02f ms]", ui_framerate, ui_frametime);

        ImGui::Text("World drawcalls: %zu", globals::num_drawcalls);
        ImGui::Text("World triangles: %zu", globals::num_triangles);
        ImGui::Text("GL_VERSION: %s", gl_version.c_str());
        ImGui::Text("GL_RENDERER: %s", gl_renderer.c_str());
        
        const ChunkCoord &cpos = view::position.chunk;
        const VoxelCoord vpos = WorldCoord::to_voxel(view::position);
        const std::intmax_t jcpos[3] = { cpos[0], cpos[1], cpos[2] };
        const std::intmax_t jvpos[3] = { vpos[0], vpos[1], vpos[2] };
        ImGui::Text("voxel: [%jd; %jd; %jd]", jvpos[0], jvpos[1], jvpos[2]);
        ImGui::Text("chunk: [%jd; %jd; %jd]", jcpos[0], jcpos[1], jcpos[2]);
    }
    
    ImGui::PopFont();
    ImGui::End();
}
