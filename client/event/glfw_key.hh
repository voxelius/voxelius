// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_EVENT_GLFW_KEYBOARD_KEY_HH
#define CLIENT_EVENT_GLFW_KEYBOARD_KEY_HH
#include <GLFW/glfw3.h>

struct GlfwKeyEvent final {
    int key {GLFW_KEY_UNKNOWN};
    int scancode {};
    int action {};
    int mods {};
};

#endif /* CLIENT_EVENT_GLFW_KEYBOARD_KEY_HH */
