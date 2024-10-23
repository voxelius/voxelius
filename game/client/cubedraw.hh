// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#pragma once
#include <game/shared/world_coord.hh>
#include <mathlib/mat4x4f.hh>
#include <mathlib/vec4f.hh>

namespace cubedraw
{
void init(void);
void deinit(void);
void render(const Mat4x4f &model, const Mat4x4f &view, float thickness = 2.0f, const Vec4f &color = Vec4f::cyan());
void render(const WorldCoord &offset, const Vec3f &size, float thickness = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace cubedraw
