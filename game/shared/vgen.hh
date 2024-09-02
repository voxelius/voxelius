// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/coord.hh>

namespace vgen
{
void init(std::uint64_t seed);
void init_late(void);
void deinit(void);
void update(void);
} // namespace vgen

namespace vgen
{
void generate(const ChunkCoord &cpos);
} // namespace vgen
