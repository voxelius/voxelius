// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/screenshot.hh>
#include <entt/signal/dispatcher.hpp>
#include <shared/epoch.hh>
#include <shared/vfs.hh>
#include <spdlog/fmt/fmt.h>
#include <stb_image_write.h>

static void png_write(void *context, void *data, int size)
{
    vfs::file_t *file = reinterpret_cast<vfs::file_t *>(context);
    vfs::write(file, data, size);
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == GLFW_KEY_F2 && event.action == GLFW_PRESS) {
        screenshot::take();
        return;
    }
}

void screenshot::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void screenshot::take()
{
    uint8_t *pixels = new uint8_t[globals::width * globals::height * 3];
    glxx::Framebuffer::unbind();
    glReadPixels(0, 0, globals::width, globals::height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    const vfs::path_t path = fmt::format("/screenshots/{}.png", epoch::microseconds());

    vfs::create_directories(path.parent_path());

    if(vfs::file_t *file = vfs::open(path, vfs::OPEN_WR)) {
        stbi_flip_vertically_on_write(true);
        stbi_write_png_to_func(&png_write, file, globals::width, globals::height, 3, pixels, 3 * globals::width);
        vfs::close(file);
    }
}
