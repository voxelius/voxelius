// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk_coord.hh>

namespace vgen
{
void init(std::uint64_t seed);
void init_late(void);
void deinit(void);
void update(void);
} // namespace vgen

namespace vgen
{
void generate(ChunkCoord::value_type cx, ChunkCoord::value_type cz);
} // namespace vgen
