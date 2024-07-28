// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_DRAW_HH
#define CLIENT_DEBUG_DRAW_HH
#include <shared/const.hh>
#include <shared/coord.hh>

namespace debug_draw
{
void init(void);
void deinit(void);
} // namespace debug_draw

namespace debug_draw
{
void begin(bool depth_testing);
void cube(const WorldPos &start, const Vector3D &scale, float width = 2.0f, const Vector4D &color = COLOR_CYAN);
void line(const WorldPos &start, const Vector3D &scale, float width = 2.0f, const Vector4D &color = COLOR_CYAN);
} // namespace debug_draw

#endif /* CLIENT_DEBUG_DRAW_HH */
