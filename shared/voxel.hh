// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VOXEL_HH
#define SHARED_VOXEL_HH
#include <cstdint>

using Voxel = std::uint16_t;
constexpr static Voxel VOXEL_MAX = UINT16_MAX;
constexpr static Voxel NULL_VOXEL = 0x0000;

#endif /* SHARED_VOXEL_HH */
