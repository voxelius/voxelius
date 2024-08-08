// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VDRAW_HH
#define CLIENT_VDRAW_HH
#include <shared/math/vec4f.hh>
#include <shared/coord.hh>

namespace outline_renderer
{
void init(void);
void deinit(void);
} // namespace outline_renderer

namespace outline_renderer
{
void begin(void);
void begin_depth(void);
void cube(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
void line(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace outline_renderer

#endif /* CLIENT_VDRAW_HH */
