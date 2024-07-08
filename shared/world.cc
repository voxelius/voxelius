// SPDX-License-Identifier: Zlib
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/chunk_update.hh>
#include <shared/globals.hh>
#include <shared/world.hh>

static emhash8::HashMap<ChunkPos, Chunk *> chunks = {};

static void vconvert_fill_single(VoxelStorage **voxels)
{
    VoxelStorage_FillSingle skel = {};
    skel.type = VoxelStorageType::FILL_SINGLE;
    skel.voxel = NULL_VOXEL;

    if(voxels[0]->type == VoxelStorageType::FILL_SINGLE) {
        // Not modifying anything
        return;
    }

    switch(voxels[0]->type) {
        case VoxelStorageType::PALETTE_UINT8:
            skel.voxel = static_cast<VoxelStorage_PaletteUint8 *>(voxels[0])->palette[0];
            break;
        case VoxelStorageType::PALETTE_UINT16:
            skel.voxel = static_cast<VoxelStorage_PaletteUint16 *>(voxels[0])->palette[0];
            break;
        case VoxelStorageType::PALETTE_UINT32:
            skel.voxel = static_cast<VoxelStorage_PaletteUint32 *>(voxels[0])->palette[0];
            break;
    }

    delete voxels[0];
    voxels[0] = new VoxelStorage_FillSingle(skel);
}

Chunk *world::create_chunk(const ChunkPos &cpos)
{

}

Chunk *world::find_chunk(const ChunkPos &cpos)
{
    const auto it = chunks.find(cpos);
    if(it != chunks.cend())
        return it->second;
    return nullptr;
}

void world::remove_chunk(const ChunkPos &cpos)
{

}

void world::purge_chunks(void)
{

}

Voxel world::get_voxel(const VoxelPos &vpos)
{

}

Voxel world::get_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{

}

void world::set_voxel(Voxel voxel, const VoxelPos &vpos)
{

}

void world::set_voxel(Voxel voxel, const ChunkPos &cpos, const LocalPos &lpos)
{

}
