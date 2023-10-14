// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozila.org/MPL/2.0/.
#include <client/event/cursor_move.hh>
#include <client/event/keyboard_key.hh>
#include <client/event/mouse_button.hh>
#include <client/event/mouse_scroll.hh>
#include <client/event/window_resize.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/image.hh>
#include <client/main.hh>
#include <glad/gl.h>
#include <shared/cmdline.hh>
#include <shared/config/config.hh>
#include <shared/epoch.hh>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static void on_glfw_error(int code, const char *message)
{
    spdlog::error("glfw: {}", message);
}

static void on_framebuffer_size(GLFWwindow *window, int width, int height)
{
    globals::window_width = width;
    globals::window_height = height;
    globals::window_aspect = static_cast<double>(width) / static_cast<double>(height);

    WindowResizeEvent event = {};
    event.width = globals::window_width;
    event.height = globals::window_height;
    event.aspect = globals::window_aspect;
    globals::dispatcher.trigger(event);
}

static void on_keyboard_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    KeyboardKeyEvent event = {};
    event.key = key;
    event.scancode = scancode;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);
}

static void on_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    MouseButtonEvent event = {};
    event.button = button;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);
}

static void on_cursor_pos(GLFWwindow *window, double xpos, double ypos)
{
    CursorMoveEvent event = {};
    event.xpos = xpos;
    event.ypos = ypos;
    globals::dispatcher.trigger(event);
}

static void on_scroll(GLFWwindow *window, double dx, double dy)
{
    MouseScrollEvent event = {};
    event.dx = dx;
    event.dx = dy;
    globals::dispatcher.trigger(event);
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

    globals::window = glfwCreateWindow(720, 480, "Client", nullptr, nullptr);

    if(!globals::window) {
        spdlog::critical("glfw: failed to open a window");
        std::terminate();
    }

    // The UI is scaled against a spherical monitor in vacuum
    // with the height of 240 pixels. The closest legal (VGA)
    // resolution we can get with that height is the crusty 320x240
    glfwSetWindowSizeLimits(globals::window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwSetFramebufferSizeCallback(globals::window, &on_framebuffer_size);
    glfwSetKeyCallback(globals::window, &on_keyboard_key);
    glfwSetMouseButtonCallback(globals::window, &on_mouse_button);
    glfwSetCursorPosCallback(globals::window, &on_cursor_pos);
    glfwSetScrollCallback(globals::window, &on_scroll);

    Image image = {};

    if(image.load_rgba("32x32.png", false)) {
        GLFWimage icon = {};
        icon.width = image.get_width();
        icon.height = image.get_height();
        icon.pixels = reinterpret_cast<unsigned char *>(image.data());
        glfwSetWindowIcon(globals::window, 1, &icon);
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

    globals::frametime = 0.0;
    globals::frametime_avg = 0.0;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    client_game::init();

    int wwidth, wheight;
    glfwGetFramebufferSize(globals::window, &wwidth, &wheight);
    on_framebuffer_size(globals::window, wwidth, wheight);

    client_game::init_late();

    config::load_file("client.conf");
    config::load_cmdline();

    uint64_t last_curtime = globals::curtime;

    while(!glfwWindowShouldClose(globals::window)) {
        globals::curtime = epoch::microseconds();
        globals::frametime = static_cast<double>(globals::curtime - last_curtime) / 1000000.0;
        globals::frametime_avg += globals::frametime;
        globals::frametime_avg *= 0.5;

        last_curtime = globals::curtime;

        client_game::update();

        glDisable(GL_BLEND);

        // Make sure there is no stray program object
        // being bound to the context. Usually third-party
        // overlay software (such as RivaTuner) injects itself
        // into the rendering loop and binds internal objects,
        // which creates a visual mess with program pipelines.
        glUseProgram(0);

        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        client_game::render();

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, globals::window_width, globals::window_height);
        client_game::render_ui();

        glfwSwapBuffers(globals::window);

        client_game::update_late();

        glfwPollEvents();

        // EnTT provides two ways of dispatching events:
        // queued and immediate. When glfwPollEvents() is
        // called, immediate events are triggered across
        // the application, whilst queued ones are triggered
        // later by calling entt::dispatcher::update()
        globals::dispatcher.update();

        globals::framecount += 1;
    }

    spdlog::info("client: shutdown after {} frames", globals::framecount);
    spdlog::info("client: average framerate: {:.03f} FPS", 1.0 / globals::frametime_avg);
    spdlog::info("client: average frametime: {:.03f} ms", 1000.0 * globals::frametime_avg);

    client_game::deinit();

    glfwDestroyWindow(globals::window);
    glfwTerminate();

    config::save_file("client.conf");
}
