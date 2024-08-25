// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>

namespace voxel_anims
{
extern std::uint64_t nextframe;
extern std::uint32_t frame;
} // namespace voxel_anims

namespace voxel_anims
{
void init(void);
void update(void);
} // namespace voxel_anims
