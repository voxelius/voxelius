#ifndef CDF3046F_AD1A_4954_8B4A_00B7AFB2A2B0
#define CDF3046F_AD1A_4954_8B4A_00B7AFB2A2B0

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"

#include "server/world/heightmap.hh"

namespace realm_sky
{
void init(std::mt19937_64& seeder);
void shutdown(void);
} // namespace realm_sky

namespace realm_sky
{
void generate(BlockStorage& blocks, BiomeSlice& biomes, const ChunkPos& pos);
} // namespace realm_sky

namespace realm_sky
{
ColumnSlice probe(const ChunkPosXZ& pos);
} // namespace realm_sky

#endif /* CDF3046F_AD1A_4954_8B4A_00B7AFB2A2B0 */
