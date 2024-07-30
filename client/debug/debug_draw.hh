// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_DEBUG_DRAW_HH
#define CLIENT_DEBUG_DEBUG_DRAW_HH
#include <shared/math/vec4f.hh>
#include <shared/world/coord.hh>

namespace debug_draw
{
void init(void);
void deinit(void);
} // namespace debug_draw

namespace debug_draw
{
void begin(bool depth_testing);
void cube(const WorldCoord &start, const Vec3f &scale, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
void line(const WorldCoord &start, const Vec3f &scale, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace debug_draw

#endif /* CLIENT_DEBUG_DEBUG_DRAW_HH */
