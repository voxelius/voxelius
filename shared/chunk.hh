// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <shared/voxel.hh>

enum class ChunkStorageType {
    Unallocated = 0xFFFF,
    SingleFill  = 0x0000,
    Palette     = 0x0001,
    Array       = 0x0002,
};

class Chunk final {
public:
    entt::entity entity {};
    ChunkStorageType storage_type {};
    void *storage {};

public:
    static void create_storage(Chunk &chunk);
    static void create_storage(Chunk &chunk, const Chunk &source);
    static void destroy_storage(Chunk &chunk);

public:
    static Voxel get_voxel(const Chunk &chunk, const LocalPos &lpos);
    static Voxel get_voxel(const Chunk &chunk, const std::size_t index);

public:
    static void set_voxel(Chunk &chunk, Voxel voxel, const LocalPos &lpos);
    static void set_voxel(Chunk &chunk, Voxel voxel, const std::size_t index);

public:
    static void fill(Chunk &chunk, Voxel voxel);
};

#endif /* SHARED_CHUNK_HH */
