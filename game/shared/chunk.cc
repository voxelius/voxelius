// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/chunk.hh>

Chunk *Chunk::create(ChunkType type)
{
    Chunk *object = new Chunk();
    object->voxels.fill(NULL_VOXEL);
    object->entity = entt::null;
    object->type = type;
    return object;
}

Chunk *Chunk::create(ChunkType type, entt::entity entity)
{
    Chunk *object = new Chunk();
    object->voxels.fill(NULL_VOXEL);
    object->entity = entity;
    object->type = type;
    return object;
}

void Chunk::destroy(Chunk *chunk)
{
    delete chunk;
}
