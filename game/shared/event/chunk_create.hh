// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>
#include <game/shared/chunk_coord.hh>

struct ChunkCreateEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};
