#include "server/pch.hh"

#include "server/world/generator_vox17.hh"

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/world.hh"

#include "server/world/climate.hh"
#include "server/world/generator_map.hh"
#include "server/world/terrain.hh"

struct Vox17_State final {
    std::vector<ClimateSeed> climate_seeds;
};

static std::shared_ptr<void> attach(World*)
{
    return std::make_shared<Vox17_State>();
}

static bool generate(World* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
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
    definition.generate = &generate;
    definition.attach = &attach;

    generator_map::detail::add("vox17", definition);
}

std::vector<ClimateSeed>& Vox17::climate_seeds(World* world)
{
    auto state = static_cast<Vox17_State*>(world->generator_state());

    assert(state);

    return state->climate_seeds;
}
