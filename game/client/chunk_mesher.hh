// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/client/chunk_vbo.hh>
#include <vector>

struct ChunkMeshComponent final {
    std::vector<ChunkVBO> quad {};
};

namespace chunk_mesher
{
void init(void);
void deinit(void);
void update(void);
} // namespace chunk_mesher
