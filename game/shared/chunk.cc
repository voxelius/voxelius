// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/chunk.hh>

Chunk *Chunk::alloc(void)
{
    return Chunk::alloc(entt::null);
}

Chunk *Chunk::alloc(entt::entity entity)
{
    Chunk *chunk = new Chunk();
    chunk->voxels.fill(NULL_VOXEL);
    chunk->entity = entity;
    return chunk;
}

void Chunk::dealloc(Chunk *chunk)
{
    delete chunk;
}
