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
void prepare_depth(void);
void prepare_nodepth(void);
void cube(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
void line(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace outline_renderer

#endif /* CLIENT_VDRAW_HH */
