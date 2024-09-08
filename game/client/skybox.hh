// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec2f.hh>
#include <mathlib/vec3f.hh>

namespace skybox
{
extern Vec3f atmosphere;
extern Vec2f sun_angles;
extern Vec2f sun_disk;
extern Vec2f horizon;
} // namespace skybox

namespace skybox
{
extern Vec3f sun_direction;
extern Vec3f fog_color;
} // namespace skybox

namespace skybox
{
void init(void);
void deinit(void);
void update(void);
void render(void);
void layout_debug(void);
} // namespace skybox
