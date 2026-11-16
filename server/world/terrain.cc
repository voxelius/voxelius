#include "server/pch.hh"

#include "server/world/terrain.hh"

#include "shared/utils/biome.hh"

#include "server/constant.hh"
#include "server/world/realm_sky.hh"
#include "server/world/realm_surface.hh"

bool terrain::generate(const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto realm = utils::realm(pos.y());
    auto generated = false;

    blocks.fill(BLOCK_ID_NULL);
    biomes.fill(BIOME_ID_NULL);

    switch(realm) {
        case BIOME_REALM_SURFACE:
            realm_surface::generate(blocks, biomes, pos);
            generated = true;
            break;

        case BIOME_REALM_SKY:
            realm_sky::generate(blocks, biomes, pos);
            generated = true;
            break;

        case BIOME_REALM_UNDERGROUND:
        case BIOME_REALM_THE_DEPTHS:
        case BIOME_REALM_VOID:
            break; // these realms are not generated yet

        case BIOME_REALM_COUNT:
            break; // shut up clang
    }

    // TODO: if generated, carve caves

    blocks.optimize();

    return generated;
}
