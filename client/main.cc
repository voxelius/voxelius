// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozila.org/MPL/2.0/.
#include <client/event/glfw_cursor_pos.hh>
#include <client/event/glfw_key.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/event/glfw_scroll.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/event/imgui_fonts_reload.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/image.hh>
#include <client/main.hh>
#include <client/ui_screen.hh>
#include <entt/signal/dispatcher.hpp>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <shared/cmdline.hh>
#include <shared/config/config.hh>
#include <shared/cxmath.hh>
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

static void on_glfw_char(GLFWwindow *window, unsigned int codepoint)
{
    if(globals::ui_screen) {
        ImGui_ImplGlfw_CharCallback(window, codepoint);
    }
}

static void on_glfw_cursor_enter(GLFWwindow *window, int entered)
{
    if(globals::ui_screen) {
        ImGui_ImplGlfw_CursorEnterCallback(window, entered);
    }
}

static void on_glfw_cursor_pos(GLFWwindow *window, double xpos, double ypos)
{
    GlfwCursorPosEvent event = {};
    event.xpos = xpos;
    event.ypos = ypos;
    globals::dispatcher.trigger(event);

    if(globals::ui_screen) {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    }
}

static void on_glfw_framebuffer_size(GLFWwindow *window, int width, int height)
{
    globals::width = width;
    globals::height = height;
    globals::aspect = static_cast<double>(width) / static_cast<double>(height);

    GlfwFramebufferSizeEvent fb_event = {};
    fb_event.width = globals::width;
    fb_event.height = globals::height;
    fb_event.aspect = globals::aspect;
    globals::dispatcher.trigger(fb_event);

    const float height_base = 240.0f;
    const float height_float = height;
    const unsigned int scale = cxmath::max(1U, cxmath::floor<unsigned int>(height_float / height_base));

    if(globals::ui_scale != scale) {
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        
        io.Fonts->Clear();
        //io.Fonts->AddFontDefault();

        ImGuiFontsReloadEvent font_event = {};
        font_event.fonts = io.Fonts;
        font_event.config.FontDataOwnedByAtlas = false;
        font_event.scale = scale;
        globals::dispatcher.trigger(font_event);

        // This should be here!!! Just calling Build()
        // on the font atlas does not invalidate internal
        // device objects defined by the implementation!!!
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        if(globals::ui_scale) {
            // Well, ImGuiStyle::ScaleAllSizes indeed takes
            // the scale values as a RELATIVE scaling, not as
            // absolute. So I have to make a special crutch
            style.ScaleAllSizes(static_cast<float>(scale) / static_cast<float>(globals::ui_scale));
        }

        globals::ui_scale = scale;
    }
}

static void on_glfw_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    GlfwKeyEvent event = {};
    event.key = key;
    event.scancode = scancode;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);

    if(globals::ui_screen) {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    }
}

static void on_glfw_monitor_event(GLFWmonitor *monitor, int event)
{
    ImGui_ImplGlfw_MonitorCallback(monitor, event);
}

static void on_glfw_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    GlfwMouseButtonEvent event = {};
    event.button = button;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);

    if(globals::ui_screen) {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    }
}

static void on_glfw_scroll(GLFWwindow *window, double dx, double dy)
{
    GlfwScrollEvent event = {};
    event.dx = dx;
    event.dx = dy;
    globals::dispatcher.trigger(event);

    if(globals::ui_screen) {
        ImGui_ImplGlfw_ScrollCallback(window, dx, dy);
    }
}

static void on_glfw_window_focus(GLFWwindow *window, int focused)
{
    ImGui_ImplGlfw_WindowFocusCallback(window, focused);
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
    // resolution we can get with that height is the crispy 320x240
    glfwSetWindowSizeLimits(globals::window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwSetCharCallback(globals::window, &on_glfw_char);
    glfwSetCursorEnterCallback(globals::window, &on_glfw_cursor_enter);
    glfwSetCursorPosCallback(globals::window, &on_glfw_cursor_pos);
    glfwSetFramebufferSizeCallback(globals::window, &on_glfw_framebuffer_size);
    glfwSetKeyCallback(globals::window, &on_glfw_key);
    glfwSetMouseButtonCallback(globals::window, &on_glfw_mouse_button);
    glfwSetScrollCallback(globals::window, &on_glfw_scroll);
    glfwSetWindowFocusCallback(globals::window, &on_glfw_window_focus);

    glfwSetMonitorCallback(&on_glfw_monitor_event);

    Image image = {};

    if(image.load_rgba("32x32.png", false)) {
        GLFWimage icon = {};
        icon.width = image.get_width();
        icon.height = image.get_height();
        icon.pixels = reinterpret_cast<unsigned char *>(image.data());
        glfwSetWindowIcon(globals::window, 1, &icon);
    }

    glfwMakeContextCurrent(globals::window);
    glfwSwapInterval(1);

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(globals::window, false);
    ImGui_ImplOpenGL3_Init(nullptr);

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    globals::frametime = 0.0;
    globals::frametime_avg = 0.0;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    globals::ui_scale = 0U;
    globals::ui_screen = ui::SCREEN_NONE;

    client_game::init();

    int wwidth, wheight;
    glfwGetFramebufferSize(globals::window, &wwidth, &wheight);
    on_glfw_framebuffer_size(globals::window, wwidth, wheight);

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        client_game::update();

        glDisable(GL_BLEND);

        // Make sure there is no stray program object
        // being bound to the context. Usually third-party
        // overlay software (such as RivaTuner) injects itself
        // into the rendering loop and binds internal objects,
        // which creates a visual mess with program pipelines.
        glUseProgram(0);

        client_game::render();

        // All the 2D rendering goes through ImGui, and it being
        // an immediate-mode solution makes it hard to separate
        // rendering and UI logic updates, so this here function
        // acts as the definitive UI rendering/logic callback
        client_game::layout();

        ImGui::Render();

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, globals::width, globals::height);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(globals::window);
    glfwTerminate();

    config::save_file("client.conf");
}
