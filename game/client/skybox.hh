// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec2f.hh>
#include <mathlib/vec3f.hh>

namespace skybox
{
extern float anisotropic_intensity;
extern float atmospheric_density;
extern float multiscatter_phase;
extern float zenith_offset;
extern unsigned int stars_gamma;
extern unsigned int stars_zoom;
extern unsigned int stars_seed;
} // namespace skybox

namespace skybox
{
extern Vec3f sky_color;
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
