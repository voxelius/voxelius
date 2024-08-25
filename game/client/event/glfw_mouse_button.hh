// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <GLFW/glfw3.h>

struct GlfwMouseButtonEvent final {
    int button {GLFW_KEY_UNKNOWN};
    int action {};
    int mods {};
};
