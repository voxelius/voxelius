#include "server/pch.hh"

#include "server/world/generator_vox2.hh"

#include "core/utils/json.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/heightmap.hh"
#include "shared/world/world.hh"

#include "server/utils/climate.hh"
#include "server/world/generator_map.hh"
#include "server/world/noise_cache_2d.hh"
#include "server/world/noise_cache_3d.hh"
#include "server/world/world.hh"

constexpr static std::size_t AXIS_COUNT = 2;
constexpr static std::array<const char*, AXIS_COUNT> AXIS_NAMES {
    "temperature",
    "humidity",
};

constexpr static BlockPos::value_type SURFACE_DEPTH_MAX = 8;
constexpr static BlockPos::value_type CHUNK_SIZE_BLOCKS = static_cast<BlockPos::value_type>(constant::CHUNK_SIZE);

struct Vox2Settings final {
    BlockPos::value_type sea_level { 64 };
    BlockPos::value_type surface_depth { 4 };
    float base_min { 44.0f };
    float base_max { 92.0f };
    float variation_min { 16.0f };
    float variation_max { 48.0f };
    float terrain_frequency { 0.03f };
    int terrain_octaves { 4 };
};

struct Vox2State final {
    Vox2Settings settings;
    std::once_flag seed_once;

    std::unique_ptr<NoiseCache2D_16x16> base_height;
    std::unique_ptr<NoiseCache2D_8x8> variation;
    std::unique_ptr<NoiseCache2D_4x4> temperature;
    std::unique_ptr<NoiseCache2D_4x4> humidity;
    std::unique_ptr<NoiseCache3D_4x8x4> detail;
};

struct ColumnPalette final {
    block_id_type base;
    block_id_type land_filler;
    block_id_type land_surface;
    block_id_type fluid;
    block_id_type shore_surface;
    block_id_type shore_filler;
};

static bool is_known_axis(std::string_view name)
{
    for(auto axis : AXIS_NAMES) {
        if(name == axis) {
            return true;
        }
    }

    return false;
}

static fnl_state make_field(std::mt19937_64& seeder, fnl_noise_type noise_type, float frequency, int octaves)
{
    fnl_state noise = fnlCreateState();

    noise.seed = static_cast<int>(seeder());
    noise.noise_type = noise_type;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.frequency = frequency;
    noise.octaves = octaves;

    return noise;
}

static void seed_state(Vox2State* state, std::uint64_t seed)
{
    std::call_once(state->seed_once, [state, seed] {
        std::mt19937_64 seeder;
        seeder.seed(seed);

        state->base_height = std::make_unique<NoiseCache2D_16x16>(make_field(seeder, FNL_NOISE_OPENSIMPLEX2S, 0.004f, 4));
        state->variation = std::make_unique<NoiseCache2D_8x8>(make_field(seeder, FNL_NOISE_OPENSIMPLEX2S, 0.008f, 3));
        state->temperature = std::make_unique<NoiseCache2D_4x4>(make_field(seeder, FNL_NOISE_OPENSIMPLEX2S, 0.010f, 3));
        state->humidity = std::make_unique<NoiseCache2D_4x4>(make_field(seeder, FNL_NOISE_OPENSIMPLEX2S, 0.014f, 3));

        auto detail = make_field(seeder, FNL_NOISE_PERLIN, state->settings.terrain_frequency, state->settings.terrain_octaves);
        state->detail = std::make_unique<NoiseCache3D_4x8x4>(detail);
    });
}

static block_id_type pick(block_id_type value, block_id_type fallback)
{
    if(value == BLOCK_ID_NULL) {
        return fallback;
    }

    return value;
}

static float column_base(const Vox2State* state, const NoiseCache2D_16x16::array_type& values, const LocalPosXZ& lpos)
{
    auto unit = utils::normalize_01(NoiseCache2D_16x16::sample(values, lpos));

    return std::lerp(state->settings.base_min, state->settings.base_max, unit);
}

static float column_variation(const Vox2State* state, const NoiseCache2D_8x8::array_type& values, const LocalPosXZ& lpos)
{
    auto unit = utils::normalize_01(NoiseCache2D_8x8::sample(values, lpos));

    return std::lerp(state->settings.variation_min, state->settings.variation_max, unit);
}

static float detail_at(const NoiseCache3D_4x8x4::array_type& values, const LocalPos& lpos)
{
    return NoiseCache3D_4x8x4::sample(values, lpos);
}

static bool is_solid(float base, float variation, float detail, BlockPos::value_type y)
{
    auto density = detail * variation - (static_cast<float>(y) - base);

    return density > 0.0f;
}

static ColumnPalette make_palette(biome_id_type biome)
{
    ColumnPalette result {};

    auto definition = biome_registry::find_definition(biome);

    if(definition == nullptr) {
        return result;
    }

    result.base = definition->palette_base.cached;
    result.land_filler = definition->palette_land_filler.cached;
    result.land_surface = definition->palette_land_surface.cached;
    result.fluid = definition->palette_fluid.cached;
    result.shore_surface = definition->palette_shore_surface.cached;
    result.shore_filler = definition->palette_shore_filler.cached;

    result.base = pick(result.base, pick(result.land_filler, result.land_surface));
    result.land_filler = pick(result.land_filler, result.base);
    result.land_surface = pick(result.land_surface, result.land_filler);
    result.shore_surface = pick(result.shore_surface, result.land_surface);
    result.shore_filler = pick(result.shore_filler, result.land_filler);

    return result;
}

static biome_id_type select_biome(const BiomeSelector& selector, float temperature, float humidity)
{
    std::array<float, AXIS_COUNT> sample {};

    for(std::size_t i = 0; i < selector.axes.size(); ++i) {
        if(selector.axes[i] == AXIS_NAMES[0]) {
            sample[i] = temperature;
        }
        else {
            sample[i] = humidity;
        }
    }

    auto view = std::span<const float>(sample.data(), selector.axes.size());

    return selector.select(view);
}

static float climate_at(const NoiseCache2D_4x4::array_type& values, const LocalPosXZ& lpos)
{
    auto unit = utils::normalize_01(NoiseCache2D_4x4::sample(values, lpos));

    return std::clamp(unit, 0.0f, 1.0f);
}

static BlockPos::value_type section_origin(ChunkPos::value_type section)
{
    return static_cast<BlockPos::value_type>(section) * CHUNK_SIZE_BLOCKS;
}

static LocalPos::value_type local_y(BlockPos::value_type y, int section)
{
    return static_cast<LocalPos::value_type>(y - (static_cast<BlockPos::value_type>(section) * CHUNK_SIZE_BLOCKS));
}

template<>
std::shared_ptr<void> Generator<Vox2>::create_state(const JSON_Object* settings, const WorldDefinition& definition, std::string_view where)
{
    Vox2Settings values;
    Vox2Settings defaults;

    if(settings) {
        if(auto value = utils::parse_arithmetic<float>(settings, "base_min")) {
            values.base_min = value.value();
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "base_max")) {
            values.base_max = value.value();
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "variation_min")) {
            values.variation_min = value.value();
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "variation_max")) {
            values.variation_max = value.value();
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "terrain_frequency")) {
            values.terrain_frequency = value.value();
        }

        if(auto value = utils::parse_integer<long long>(settings, "terrain_octaves")) {
            values.terrain_octaves = static_cast<int>(value.value());
        }

        if(auto value = utils::parse_integer<long long>(settings, "sea_level")) {
            values.sea_level = static_cast<BlockPos::value_type>(value.value());
        }

        if(auto value = utils::parse_integer<long long>(settings, "surface_depth")) {
            values.surface_depth = static_cast<BlockPos::value_type>(value.value());
        }
    }

    auto base_valid = std::isfinite(values.base_min) && std::isfinite(values.base_max) && values.base_min <= values.base_max;

    if(!base_valid) {
        msg::log_warning("{}: base_min and base_max are invalid; the defaults are used", where);
        values.base_min = defaults.base_min;
        values.base_max = defaults.base_max;
    }

    auto world_min = static_cast<float>(definition.min_y);
    auto world_max = static_cast<float>(definition.max_y);

    values.base_min = std::clamp(values.base_min, world_min, world_max);
    values.base_max = std::clamp(values.base_max, world_min, world_max);

    auto variation_valid = true;
    variation_valid = variation_valid && std::isfinite(values.variation_min) && std::isfinite(values.variation_max);
    variation_valid = variation_valid && values.variation_min >= 0.0f && values.variation_min <= values.variation_max;

    if(!variation_valid) {
        msg::log_warning("{}: variation_min and variation_max are invalid; the defaults are used", where);
        values.variation_min = defaults.variation_min;
        values.variation_max = defaults.variation_max;
    }

    auto variation_limit = static_cast<float>(definition.max_y - definition.min_y);

    values.variation_min = std::clamp(values.variation_min, 0.0f, variation_limit);
    values.variation_max = std::clamp(values.variation_max, 0.0f, variation_limit);

    if(!std::isfinite(values.terrain_frequency) || values.terrain_frequency <= 0.0f) {
        msg::log_warning("{}: terrain_frequency is invalid; the default is used", where);
        values.terrain_frequency = defaults.terrain_frequency;
    }

    if(values.terrain_octaves < 1 || values.terrain_octaves > 8) {
        msg::log_warning("{}: terrain_octaves is invalid; the default is used", where);
        values.terrain_octaves = defaults.terrain_octaves;
    }

    if(values.surface_depth < 0 || values.surface_depth > SURFACE_DEPTH_MAX) {
        msg::log_warning("{}: surface_depth is invalid; the default is used", where);
        values.surface_depth = defaults.surface_depth;
    }

    if(values.sea_level < definition.min_y || values.sea_level > definition.max_y) {
        msg::log_warning("{}: sea_level is outside the world height; it is clamped", where);
        values.sea_level = std::clamp(values.sea_level, definition.min_y, definition.max_y);
    }

    auto& selector = definition.biome_selector;

    if(selector.strategy == BIOME_SELECTION_NEAREST) {
        for(std::size_t i = 0; i < selector.axes.size(); ++i) {
            if(!is_known_axis(selector.axes[i])) {
                msg::log_error("{}: vox2 does not know the {} biome axis; the world is skipped", where, selector.axes[i]);
                return nullptr;
            }

            for(std::size_t j = 0; j < i; ++j) {
                if(selector.axes[i] == selector.axes[j]) {
                    msg::log_error("{}: the {} biome axis is repeated; the world is skipped", where, selector.axes[i]);
                    return nullptr;
                }
            }
        }

        if(selector.axes.empty()) {
            msg::log_error("{}: vox2 needs at least one biome axis; the world is skipped", where);
            return nullptr;
        }
    }
    else if(selector.strategy == BIOME_SELECTION_GRID) {
        if(selector.axes.size() != 2) {
            msg::log_error("{}: vox2 needs exactly two biome axes; the world is skipped", where);
            return nullptr;
        }

        for(auto& axis : selector.axes) {
            if(!is_known_axis(axis)) {
                msg::log_error("{}: vox2 does not know the {} biome axis; the world is skipped", where, axis);
                return nullptr;
            }
        }

        if(selector.axes[0] == selector.axes[1]) {
            msg::log_error("{}: vox2 needs two different biome axes; the world is skipped", where);
            return nullptr;
        }
    }
    else if(selector.strategy != BIOME_SELECTION_CONSTANT) {
        msg::log_error("{}: vox2 does not support this biome selector; the world is skipped", where);
        return nullptr;
    }

    auto state = std::make_shared<Vox2State>();
    state->settings = values;

    return state;
}

template<>
void Generator<Vox2>::generate_biomes(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out)
{
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto& definition = world->definition();
    auto& selector = definition.biome_selector;
    auto& temperature_values = state->temperature->get(pos);
    auto& humidity_values = state->humidity->get(pos);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos = utils::to_local_xz(i);
        auto temperature = climate_at(temperature_values, lpos);
        auto humidity = climate_at(humidity_values, lpos);

        out[i] = select_biome(selector, temperature, humidity);
    }
}

template<>
ColumnSlice Generator<Vox2>::sample_heightmap(ServerWorld* world, const ChunkPosXZ& pos)
{
    auto& definition = world->definition();
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto min_section = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_section = definition.max_y >> constant::CHUNK_SIZE_LOG2;
    auto section_count = static_cast<std::size_t>(max_section - min_section + 1);

    std::vector<const NoiseCache3D_4x8x4::array_type*> detail(section_count);

    for(auto section = min_section; section <= max_section; section += 1) {
        auto index = static_cast<std::size_t>(section - min_section);
        auto cpos = ChunkPos(pos.x(), static_cast<ChunkPos::value_type>(section), pos.y());
        detail[index] = &state->detail->get(cpos);
    }

    auto& base_values = state->base_height->get(pos);
    auto& variation_values = state->variation->get(pos);

    auto water_top = state->settings.sea_level - 1;
    auto has_water = water_top >= definition.min_y && water_top <= definition.max_y;

    ColumnSlice slice {};

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos_xz = utils::to_local_xz(i);
        auto base = column_base(state, base_values, lpos_xz);
        auto variation = column_variation(state, variation_values, lpos_xz);

        auto surface = Column::UNSET;

        for(auto y = definition.max_y; y >= definition.min_y; --y) {
            auto section = static_cast<int>(y >> constant::CHUNK_SIZE_LOG2);
            auto index = static_cast<std::size_t>(section - min_section);
            auto& values = *detail[index];
            auto ly = local_y(y, section);
            LocalPos lpos(lpos_xz.x(), ly, lpos_xz.y());
            auto detail_value = detail_at(values, lpos);

            if(is_solid(base, variation, detail_value, y)) {
                surface = y;
                break;
            }
        }

        auto liquid = Column::UNSET;

        if(has_water) {
            auto section = static_cast<int>(water_top >> constant::CHUNK_SIZE_LOG2);
            auto index = static_cast<std::size_t>(section - min_section);
            auto& values = *detail[index];
            auto ly = local_y(water_top, section);
            LocalPos lpos(lpos_xz.x(), ly, lpos_xz.y());
            auto detail_value = detail_at(values, lpos);

            if(!is_solid(base, variation, detail_value, water_top)) {
                liquid = water_top;
            }
        }

        slice[i].surface_y = surface;
        slice[i].liquid_y = liquid;
    }

    return slice;
}

template<>
bool Generator<Vox2>::generate_blocks(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto& definition = world->definition();
    auto min_section = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_section = definition.max_y >> constant::CHUNK_SIZE_LOG2;

    if(pos.y() < min_section || pos.y() > max_section) {
        return false;
    }

    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto cpos_xz = ChunkPosXZ(pos.x(), pos.z());

    Generator<Vox2>::generate_biomes(world, cpos_xz, biomes);

    auto& base_values = state->base_height->get(cpos_xz);
    auto& variation_values = state->variation->get(cpos_xz);

    std::array<ColumnPalette, constant::CHUNK_AREA> palette {};

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        palette[i] = make_palette(biomes[i]);
    }

    auto& detail = state->detail->get(pos);
    const NoiseCache3D_4x8x4::array_type* detail_above = nullptr;

    if(pos.y() + 1 <= max_section) {
        auto above = ChunkPos(pos.x(), pos.y() + 1, pos.z());
        detail_above = &state->detail->get(above);
    }

    auto section_bottom = section_origin(pos.y());
    auto section_top = section_bottom + CHUNK_SIZE_BLOCKS - 1;

    auto sea_level = state->settings.sea_level;
    auto surface_depth = state->settings.surface_depth;

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos_xz = utils::to_local_xz(i);
        auto base = column_base(state, base_values, lpos_xz);
        auto variation = column_variation(state, variation_values, lpos_xz);
        auto& column = palette[i];

        auto entering_top = Column::UNSET;

        if(detail_above) {
            auto above_origin = section_top + 1;
            auto scan_limit = section_top + surface_depth + 1;
            auto y = above_origin;

            while(y <= scan_limit) {
                auto ly = local_y(y, pos.y() + 1);
                LocalPos lpos(lpos_xz.x(), ly, lpos_xz.y());
                auto detail_value = detail_at(*detail_above, lpos);

                if(!is_solid(base, variation, detail_value, y)) {
                    break;
                }

                y += 1;
            }

            if(y > above_origin) {
                if(y <= scan_limit) {
                    entering_top = y - 1;
                }
                else {
                    entering_top = scan_limit;
                }
            }
        }

        auto run_top = entering_top;

        for(auto y = section_top; y >= section_bottom; --y) {
            auto ly = local_y(y, pos.y());
            LocalPos lpos(lpos_xz.x(), ly, lpos_xz.y());
            auto index = utils::to_index(lpos);
            auto detail_value = detail_at(detail, lpos);

            if(!is_solid(base, variation, detail_value, y)) {
                run_top = Column::UNSET;

                if(y < sea_level) {
                    blocks.set(index, column.fluid);
                }

                continue;
            }

            if(run_top == Column::UNSET) {
                run_top = y;
            }

            auto depth = run_top - y;

            if(depth == 0) {
                auto beach = y >= sea_level - 4 && y <= sea_level + 1;
                auto top = column.land_surface;
                auto fill = column.land_filler;

                if(beach) {
                    top = column.shore_surface;
                    fill = column.shore_filler;
                }

                if(y >= sea_level - 1) {
                    blocks.set(index, top);
                }
                else {
                    blocks.set(index, fill);
                }
            }
            else if(depth <= surface_depth) {
                blocks.set(index, column.land_filler);
            }
            else {
                blocks.set(index, column.base);
            }
        }
    }

    world->commit_placements(blocks, pos);

    return true;
}

template<>
block_id_type Generator<Vox2>::surface_block(ServerWorld* world, const BlockPosXZ& pos, const Column& column)
{
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    auto& settings = state->settings;
    auto biome = world->biome_at(pos);
    auto palette = make_palette(biome);
    auto surface_y = column.surface_y;
    auto beach = surface_y >= settings.sea_level - 4 && surface_y <= settings.sea_level + 1;

    if(surface_y >= settings.sea_level - 1) {
        if(beach) {
            return palette.shore_surface;
        }

        return palette.land_surface;
    }

    if(beach) {
        return palette.shore_filler;
    }

    return palette.land_filler;
}

void Vox2::register_generator(void)
{
    generator_map::add<Vox2>("vox2");
}
