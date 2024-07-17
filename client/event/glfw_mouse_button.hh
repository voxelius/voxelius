// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_EVENT_GLFW_MOUSE_BUTTON_HH
#define CLIENT_EVENT_GLFW_MOUSE_BUTTON_HH
#include <GLFW/glfw3.h>

struct GlfwMouseButtonEvent final {
    int button {GLFW_KEY_UNKNOWN};
    int action {};
    int mods {};
};

#endif /* CLIENT_EVENT_GLFW_MOUSE_BUTTON_HH */
