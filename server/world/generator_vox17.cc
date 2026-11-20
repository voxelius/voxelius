#include "server/pch.hh"

#include "server/world/generator_vox17.hh"

#include "server/world/world.hh"

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/world.hh"

#include "server/world/climate.hh"
#include "server/world/generator_map.hh"
#include "server/world/terrain.hh"

// Vox17-specific generation state lands here
struct Vox17_State final {};

static std::shared_ptr<void> attach(ServerWorld*)
{
    return std::make_shared<Vox17_State>();
}

static bool generate(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto& definition = world->definition();
    auto min_y = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_y = definition.max_y >> constant::CHUNK_SIZE_LOG2;

    if(pos.y() < min_y || pos.y() > max_y) {
        return false;
    }

    return terrain::generate(world, pos, blocks, biomes);
}

void Vox17::register_generator(void)
{
    GeneratorDefinition definition {};
    definition.attach = &attach;
    definition.generate = &generate;

    generator_map::detail::add("vox17", definition);
}
