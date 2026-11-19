#ifndef A798F450_19DF_487D_82C7_AA3511C9328A
#define A798F450_19DF_487D_82C7_AA3511C9328A

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"

#include "server/world/heightmap.hh"

namespace realm_surface
{
void init(std::mt19937_64& seeder);
void shutdown(void);
} // namespace realm_surface

namespace realm_surface
{
void generate(BlockStorage& blocks, BiomeSlice& biomes, const ChunkPos& pos);
} // namespace realm_surface

namespace realm_surface
{
ColumnSlice probe(const ChunkPosXZ& pos);
} // namespace realm_surface

#endif /* A798F450_19DF_487D_82C7_AA3511C9328A */
