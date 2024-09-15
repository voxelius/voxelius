// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec2f.hh>
#include <mathlib/vec3f.hh>

namespace skybox
{
extern Vec3f absorption;
extern float horizon_scale;
extern float horizon_start;
extern float horizon_end;
extern float stars_gamma;
extern unsigned int stars_scale;
extern unsigned int stars_seed;
extern float twilight_offset;
extern Vec2f sun_angles;
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
