// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#pragma once
#include <game/shared/world_coord.hh>
#include <mathlib/mat4x4f.hh>
#include <mathlib/vec4f.hh>

namespace outline
{
void init(void);
void deinit(void);
void prepare(void);
void cube(const WorldCoord &offset, const Vec3f &size, float thickness = 1.0f, const Vec4f &color = Vec4f::cyan());
void line(const WorldCoord &offset, const Vec3f &line, float thickness = 1.0f, const Vec4f &color = Vec4f::cyan());
} // namespace outline
