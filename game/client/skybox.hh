// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec2f.hh>
#include <mathlib/vec3f.hh>

namespace skybox
{
extern Vec2f sun_angles;
extern Vec3f sun_direction;
} // namespace skybox

namespace skybox
{
extern float horizon_x;
extern float horizon_y;
} // namespace skybox

namespace skybox
{
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
