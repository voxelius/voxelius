#include "server/pch.hh"

#include "server/world/terrain.hh"

#include "shared/world/world.hh"

#include "server/constant.hh"
#include "server/world/realm_surface.hh"

bool terrain::generate(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    blocks.fill(BLOCK_ID_NULL);
    biomes.fill(BIOME_ID_NULL);

    realm_surface::generate(world, blocks, biomes, pos);

    // TODO: carve caves

    blocks.optimize();

    return true;
}
