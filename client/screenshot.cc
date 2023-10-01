// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/input.hh>
#include <client/screen.hh>
#include <client/screenshot.hh>
#include <shared/epoch.hh>
#include <shared/vfs.hh>
#include <stb_image_write.h>
#include <spdlog/spdlog.h>

static void png_write(void *context, void *data, int size)
{
    vfs::file_t *file = reinterpret_cast<vfs::file_t *>(context);
    vfs::write(file, data, size);
}

static void on_key(const KeyEvent &event)
{
    if(event.action == GLFW_PRESS && event.key == GLFW_KEY_F2) {
        int width, height;
        screen::get_size(width, height);

        uint8_t *pixels = new uint8_t[3 * width * height];
        glxx::Framebuffer::unbind();
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        const vfs::path_t path = fmt::format("/screenshots/{}.png", epoch::microseconds());

        vfs::create_directories(path.parent_path());

        if(vfs::file_t *file = vfs::open(path, vfs::OPEN_WR)) {
            stbi_flip_vertically_on_write(true);
            stbi_write_png_to_func(&png_write, file, width, height, 3, pixels, 3 * width);
            spdlog::info("screenshot: wrote {}", path.string());
            vfs::close(file);
        }
    }
}

void screenshot::init()
{
    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}
