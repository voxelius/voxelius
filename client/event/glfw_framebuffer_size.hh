// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_EVENT_GLFW_WINDOW_RESIZE_HH
#define CLIENT_EVENT_GLFW_WINDOW_RESIZE_HH

struct GlfwFramebufferSizeEvent final {
    int width {};
    int height {};
    float aspect {};
};

#endif /* CLIENT_EVENT_GLFW_WINDOW_RESIZE_HH */
