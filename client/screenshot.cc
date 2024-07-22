// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/screenshot.hh>
#include <entt/signal/dispatcher.hpp>
#include <shared/util/epoch.hh>
#include <shared/util/physfs.hh>
#include <spdlog/fmt/fmt.h>
#include <stb_image_write.h>

static void png_write(void *context, void *data, int size)
{
    PHYSFS_File *file = reinterpret_cast<PHYSFS_File *>(context);
    PHYSFS_writeBytes(file, data, size);
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
    std::uint8_t *pixels = new std::uint8_t[globals::width * globals::height * 3];
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadPixels(0, 0, globals::width, globals::height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    const std::string dirname = std::string("/screenshots");
    const std::string path = fmt::format("{}/{}.png", dirname, util::epoch_microseconds());

    PHYSFS_mkdir(dirname.c_str());

    if(PHYSFS_File *file = PHYSFS_openWrite(path.c_str())) {
        stbi_flip_vertically_on_write(true);
        stbi_write_png_to_func(&png_write, file, globals::width, globals::height, 3, pixels, 3 * globals::width);
        PHYSFS_close(file);
    }
}