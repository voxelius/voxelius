// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_DEBUG_DRAW_HH
#define CLIENT_DEBUG_DRAW_HH
#include <shared/math/vector4D.hh>
#include <shared/world/coord.hh>

namespace debug_draw
{
void init(void);
void deinit(void);
} // namespace debug_draw

namespace debug_draw
{
void begin(bool depth_testing);
void cube(const WorldCoord &start, const Vector3D &scale, float width = 2.0f, const Vector4D &color = Vector4D::cyan());
void line(const WorldCoord &start, const Vector3D &scale, float width = 2.0f, const Vector4D &color = Vector4D::cyan());
} // namespace debug_draw

#endif /* CLIENT_DEBUG_DRAW_HH */
