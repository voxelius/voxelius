// SPDX-License-Identifier: Zlib
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/globals.hh>
#include <shared/world.hh>

struct PaletteStorageU8 final {
    std::array<std::uint8_t, CHUNK_VOLUME> swatch {};
    std::vector<Voxel> palette {};
};

struct PaletteStorageU16 final {
    std::array<std::uint16_t, CHUNK_VOLUME> swatch {};
    std::vector<Voxel> palette {};
};

static emhash8::HashMap<ChunkCoord, Chunk *> chunks = {};

Chunk *world::create_chunk(const ChunkCoord &cv)
{
    Chunk *chunk = world::find_chunk(cv);

    if(chunk == nullptr) {
        chunk = new Chunk();
        chunk->entity = globals::registry.create();
        chunk->type = ChunkType::SINGLE_VOXEL;
        chunk->voxels = reinterpret_cast<void *>(NULL_VOXEL);

        // TODO

    }

}

Chunk *world::find_chunk(const ChunkCoord &cv)
{

}

void world::remove_chunk(const ChunkCoord &cv)
{

}

void world::purge_chunks(void)
{

}

Voxel world::get_voxel(const VoxelCoord &vv)
{

}

Voxel world::get_voxel(const ChunkCoord &cv, const LocalCoord &lv)
{

}

void world::set_voxel(Voxel voxel, const VoxelCoord &vv)
{

}

void world::set_voxel(Voxel voxel, const ChunkCoord &cv, const LocalCoord &lv)
{

}

void world::fill_chunks(Voxel voxel, const ChunkCoord &cvx, const ChunkCoord &cvy)
{

}

void world::fill_voxels(Voxel voxel, const VoxelCoord &vvx, const VoxelCoord &vvy)
{

}
