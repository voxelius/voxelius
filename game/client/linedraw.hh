// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#pragma once
#include <game/shared/world_coord.hh>
#include <mathlib/vec4f.hh>

namespace linedraw
{
void init(void);
void deinit(void);
void render(const WorldCoord &offset, const Vec3f &line, float thickness = 2.0f, const Vec4f &color = Vec4f::cyan());
} // namespace linedraw
