// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_OUTLINE_RENDERER_HH
#define CLIENT_OUTLINE_RENDERER_HH
#include <shared/math/vec4f.hh>
#include <shared/coord.hh>

namespace outline_renderer
{
void init(void);
void deinit(void);
void prepare_depth(void);
void prepare_nodepth(void);
} // namespace outline_renderer

namespace outline_renderer
{
void draw_cube(const WorldCoord &wpos, const Vec3f &size, float width, const Vec4f &color);
void draw_line(const WorldCoord &wpos, const Vec3f &size, float width, const Vec4f &color);
} // namespace outline_renderer

#endif /* CLIENT_OUTLINE_RENDERER_HH */
