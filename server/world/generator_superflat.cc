#include "server/pch.hh"

#include "server/world/generator_superflat.hh"

#include "core/exception.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

#include "server/world/generator_map.hh"
#include "server/world/world.hh"

constexpr static BlockPos::value_type SURFACE_Y = 0;
constexpr static BlockPos::value_type FILLER_DEPTH = 3;

struct Superflat_State final {
    // empty
};

template<>
std::shared_ptr<void> Generator<Superflat>::attach(ServerWorld* world)
{
    auto& definition = world->definition();
    auto& selector = definition.biome_selector;

    vx::throw_if(selector.strategy != BIOME_SELECTION_CONSTANT, "superflat requires a constant biome selector");

    return std::make_shared<Superflat_State>();
}

template<>
ColumnSlice Generator<Superflat>::probe(ServerWorld* world, const ChunkPosXZ& pos)
{
    Column column {};
    column.surface_y = SURFACE_Y;

    ColumnSlice slice;
    slice.fill(column);

    return slice;
}

template<>
void Generator<Superflat>::fill_biomes(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out)
{
    auto& definition = world->definition();
    auto& selector = definition.biome_selector;

    out.fill(selector.constant_biome);
}

template<>
bool Generator<Superflat>::generate(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto& definition = world->definition();
    auto min_y = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_y = definition.max_y >> constant::CHUNK_SIZE_LOG2;

    if(pos.y() < min_y || pos.y() > max_y) {
        return false;
    }

    auto& selector = definition.biome_selector;
    auto biome = selector.constant_biome;
    auto biome_def = biome_registry::find_definition(biome);

    biomes.fill(biome);

    auto surface = BLOCK_ID_NULL;
    auto filler = BLOCK_ID_NULL;
    auto basic = BLOCK_ID_NULL;

    if(biome_def) {
        surface = biome_def->palette_surface.cached;
        filler = biome_def->palette_filler.cached;
        basic = biome_def->palette_basic.cached;
    }

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto y = bpos.y();

        if(y > SURFACE_Y) {
            continue;
        }

        if(y == SURFACE_Y) {
            blocks.set(i, surface);
        }
        else if(y >= SURFACE_Y - FILLER_DEPTH) {
            blocks.set(i, filler);
        }
        else {
            blocks.set(i, basic);
        }
    }

    return true;
}

void Superflat::register_generator(void)
{
    generator_map::add<Superflat>("superflat");
}
