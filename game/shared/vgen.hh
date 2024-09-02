// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/coord.hh>

namespace vgen
{
void init(std::uint64_t seed);
void init_late(void);
} // namespace vgen

namespace vgen
{
void gen_islands(ChunkCoord::value_type cx, ChunkCoord::value_type cz);
void gen_overworld(ChunkCoord::value_type cx, ChunkCoord::value_type cz);
void gen_nether(ChunkCoord::value_type cx, ChunkCoord::value_type cz);
} // namespace vgen
