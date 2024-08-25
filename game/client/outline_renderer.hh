// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/coord.hh>
#include <mathlib/vec4f.hh>

namespace outline_renderer
{
void init(void);
void deinit(void);
void prepare_depth(void);
void prepare_nodepth(void);
void cube(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
void line(const WorldCoord &wvec, const Vec3f &size, float width = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace outline_renderer
