#include "server/pch.hh"

#include "server/world/generator_superflat.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

#include "server/world/generator_map.hh"
#include "server/world/generator_settings.hh"
#include "server/world/world.hh"

constexpr static std::array<const char*, 2> ALLOWED_KEYS {
    "surface_y",
    "filler_depth",
};

struct SuperflatSettings final {
    BlockPos::value_type surface_y { 0 };
    BlockPos::value_type filler_depth { 3 };
};

struct SuperflatState final {
    SuperflatSettings settings;
};

template<>
std::shared_ptr<void> Generator<Superflat>::create_state(lua_State* L, int settings_idx, const WorldDefinition& definition)
{
    SuperflatSettings settings;

    if(settings_idx) {
        if(!generator_settings::check_keys(L, settings_idx, "superflat", ALLOWED_KEYS)) {
            return nullptr;
        }

        auto surface_y = static_cast<lua_Integer>(settings.surface_y);

        if(!generator_settings::read_integer(L, settings_idx, "superflat", "surface_y", surface_y)) {
            return nullptr;
        }

        settings.surface_y = static_cast<BlockPos::value_type>(surface_y);

        auto filler_depth = static_cast<lua_Integer>(settings.filler_depth);

        if(!generator_settings::read_integer(L, settings_idx, "superflat", "filler_depth", filler_depth)) {
            return nullptr;
        }

        settings.filler_depth = static_cast<BlockPos::value_type>(filler_depth);
    }

    if(settings.surface_y < definition.min_y || settings.surface_y > definition.max_y) {
        lua_pushliteral(L, "superflat: surface_y is outside the world height");
        return nullptr;
    }

    if(settings.filler_depth < 0) {
        lua_pushliteral(L, "superflat: filler_depth must not be negative");
        return nullptr;
    }

    auto headroom = static_cast<std::uint64_t>(settings.surface_y) - static_cast<std::uint64_t>(definition.min_y);

    if(static_cast<std::uint64_t>(settings.filler_depth) > headroom) {
        lua_pushliteral(L, "superflat: filler_depth reaches below the world");
        return nullptr;
    }

    if(definition.biome_selector.strategy != BIOME_SELECTION_CONSTANT) {
        lua_pushliteral(L, "superflat: biome selector must be constant");
        return nullptr;
    }

    auto state = std::make_shared<SuperflatState>();
    state->settings = settings;

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
        surface = biome_def->palette_surface.cached;
        filler = biome_def->palette_filler.cached;
        basic = biome_def->palette_basic.cached;
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

void Superflat::register_generator(void)
{
    generator_map::add<Superflat>("superflat");
}
