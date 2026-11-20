#ifndef F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11
#define F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"

class World;

struct GeneratorDefinition final {
    bool (*generate)(World* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes);
    std::shared_ptr<void> (*attach)(World* world);
};

namespace generator_map::detail
{
void add(std::string_view name, GeneratorDefinition definition);
} // namespace generator_map::detail

namespace generator_map
{
const GeneratorDefinition* find(std::string_view name);
} // namespace generator_map

#endif /* F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11 */
