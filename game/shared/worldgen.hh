// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/coord.hh>

namespace worldgen
{
void init(std::uint64_t seed);
void deinit(void);
void update(void);
} // namespace worldgen

namespace worldgen
{
bool generate(const ChunkCoord &cpos);
} // namespace worldgen
