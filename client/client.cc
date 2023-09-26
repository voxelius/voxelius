// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/client.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <core/epoch.hh>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

static void on_glfw_error(int code, const char *message)
{
    spdlog::error("glfw: {}", message);
}

static void on_opengl_message(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char *message, const void *param)
{
    spdlog::info("opengl: {}", message);
}

void client::main()
{
    glfwSetErrorCallback(&on_glfw_error);

    if(!glfwInit()) {
        spdlog::critical("glfw: init failed");
        std::terminate();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_SAMPLES, 0);

    globals::window = glfwCreateWindow(640, 480, "Client", nullptr, nullptr);

    if(!globals::window) {
        spdlog::critical("glfw: failed to open a window");
        std::terminate();
    }

    glfwMakeContextCurrent(globals::window);

    if(!gladLoadGL(&glfwGetProcAddress)) {
        spdlog::critical("glad: failed to load function pointers");
        std::terminate();
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&on_opengl_message, nullptr);

    // NVIDIA drivers print additional buffer information
    // to the debug output that programmers might find useful.
    // The problem with that is that THIS FLOODS THE CONSOLE.
    static const uint32_t ignore_nvidia_131185 = 131185;
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 1, &ignore_nvidia_131185, GL_FALSE);

    spdlog::info("opengl: version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    spdlog::info("opengl: renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    // Make sure we don't have this
    // war crime of a feature enabled.
    glDisable(GL_MULTISAMPLE);

    globals::frametime = 0.0f;
    globals::frametime_avg = 0.0f;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    client_game::init();
    client_game::init_late();

    // UNDONE: we might want to calculate frametime
    // in the "native" microsecond ticks so it's a good
    // idea to also store last_curtime as a globalvar.
    uint64_t last_curtime = globals::curtime;

    while(!glfwWindowShouldClose(globals::window)) {
        globals::curtime = epoch::microseconds();
        globals::frametime = static_cast<float>(globals::curtime - last_curtime) / 1000000.0f;
        globals::frametime_avg += globals::frametime;
        globals::frametime_avg *= 0.5f;

        last_curtime = globals::curtime;

        client_game::update();

        // Make sure there is no stray program object
        // being bound to the context. Usually third-party
        // overlay software (such as RivaTuner) injects itself
        // into the rendering loop and binds internal objects,
        // which creates a visual mess with program pipelines.
        glUseProgram(0);

        client_game::render();

        glfwSwapBuffers(globals::window);

        client_game::update_late();

        glfwPollEvents();

        // EnTT provides two ways of dispatching events:
        // queued and immediate. When glfwPollEvents() is
        // called, immediate events are triggered across
        // the application, whilst queued ones are triggered
        // later by calling entt::dispatcher::update()
        globals::dispatcher.update();

        globals::framecount++;
    }

    spdlog::info("client: shutdown after {} frames", globals::framecount);
    spdlog::info("client: average framerate: {:.03f} FPS", 1.0 / globals::frametime_avg);
    spdlog::info("client: average frametime: {:.03f} ms", 1000.0 * globals::frametime_avg);

    client_game::deinit();

    glfwDestroyWindow(globals::window);
    glfwTerminate();
}
