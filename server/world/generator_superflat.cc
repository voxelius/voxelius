#include "server/pch.hh"

#include "server/world/generator_superflat.hh"

#include "core/utils/yaml.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

#include "server/world/generator_map.hh"
#include "server/world/world.hh"

struct SuperflatSettings final {
    BlockPos::value_type surface_y { 0 };
    BlockPos::value_type filler_depth { 3 };
};

struct SuperflatState final {
    SuperflatSettings settings;
};

template<>
std::shared_ptr<void> Generator<Superflat>::create_state(ryml::ConstNodeRef settings, const WorldDefinition& definition,
    std::string_view where)
{
    SuperflatSettings values;
    SuperflatSettings defaults;

    if(auto value = utils::parse_integer<long long>(settings, "surface_y")) {
        values.surface_y = static_cast<BlockPos::value_type>(value.value());
    }

    if(auto value = utils::parse_integer<long long>(settings, "filler_depth")) {
        values.filler_depth = static_cast<BlockPos::value_type>(value.value());
    }

    if(values.surface_y < definition.min_y || values.surface_y > definition.max_y) {
        msg::log_warning("{}: surface_y is outside the world height; it is clamped", where);
        values.surface_y = std::clamp(values.surface_y, definition.min_y, definition.max_y);
    }

    if(values.filler_depth < 0) {
        msg::log_warning("{}: filler_depth is invalid; the default is used", where);
        values.filler_depth = defaults.filler_depth;
    }

    auto headroom = static_cast<std::uint64_t>(values.surface_y) - static_cast<std::uint64_t>(definition.min_y);

    if(static_cast<std::uint64_t>(values.filler_depth) > headroom) {
        msg::log_warning("{}: filler_depth reaches below the world; it is clamped", where);
        values.filler_depth = static_cast<BlockPos::value_type>(headroom);
    }

    if(definition.biome_selector.strategy != BIOME_SELECTION_CONSTANT) {
        msg::log_error("{}: superflat needs a constant biome selector; the world is skipped", where);
        return nullptr;
    }

    auto state = std::make_shared<SuperflatState>();
    state->settings = values;

    return state;
}

template<>
ColumnSlice Generator<Superflat>::sample_heightmap(ServerWorld* world, const ChunkPosXZ&)
{
    auto state = static_cast<SuperflatState*>(world->generator_state());

    assert(state);

    Column column {};
    column.surface_y = state->settings.surface_y;

    ColumnSlice slice;
    slice.fill(column);

    return slice;
}

template<>
void Generator<Superflat>::generate_biomes(ServerWorld* world, const ChunkPosXZ&, BiomeSlice& out)
{
    auto& definition = world->definition();
    auto& selector = definition.biome_selector;

    out.fill(selector.constant_biome);
}

template<>
bool Generator<Superflat>::generate_blocks(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto& definition = world->definition();
    auto min_y = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_y = definition.max_y >> constant::CHUNK_SIZE_LOG2;

    if(pos.y() < min_y || pos.y() > max_y) {
        return false;
    }

    auto state = static_cast<SuperflatState*>(world->generator_state());

    assert(state);

    auto& selector = definition.biome_selector;
    auto biome = selector.constant_biome;
    auto biome_def = biome_registry::find_definition(biome);

    biomes.fill(biome);

    auto surface_y = state->settings.surface_y;
    auto filler_depth = state->settings.filler_depth;

    auto surface = BLOCK_ID_NULL;
    auto filler = BLOCK_ID_NULL;
    auto basic = BLOCK_ID_NULL;

    if(biome_def) {
        surface = biome_def->palette_land_surface.cached;
        filler = biome_def->palette_land_filler.cached;
        basic = biome_def->palette_base.cached;
    }

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto y = bpos.y();

        if(y > surface_y) {
            continue;
        }

        if(y == surface_y) {
            blocks.set(i, surface);
        }
        else if(y >= surface_y - filler_depth) {
            blocks.set(i, filler);
        }
        else {
            blocks.set(i, basic);
        }
    }

    return true;
}

template<>
block_id_type Generator<Superflat>::surface_block(ServerWorld* world, const BlockPosXZ& pos, const Column& column)
{
    auto& definition = world->definition();
    auto biome_def = biome_registry::find_definition(definition.biome_selector.constant_biome);

    if(biome_def) {
        return biome_def->palette_land_surface.cached;
    }

    return BLOCK_ID_NULL;
}

void Superflat::register_generator(void)
{
    generator_map::add<Superflat>("superflat");
}
