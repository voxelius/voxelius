// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/skybox.hh>

Vec3f skybox::fog_color = Vec3f(0.0f, 0.0f, 0.0f);

void skybox::init(void)
{
    // https://convertingcolors.com/hex-color-B1F3FF.html
    skybox::fog_color = Vec3f(0.690f, 0.950f, 1.000f);
}
