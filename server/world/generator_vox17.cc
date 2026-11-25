#include "server/pch.hh"

#include "server/world/generator_vox17.hh"

#include "core/utils/json.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/heightmap.hh"
#include "shared/world/world.hh"

#include "server/utils/climate.hh"
#include "server/world/generator_map.hh"
#include "server/world/noise_cache_2d.hh"
#include "server/world/noise_cache_3d.hh"
#include "server/world/world.hh"

constexpr static LocalPos::value_type CHUNK_SIZE_LP = static_cast<LocalPos::value_type>(constant::CHUNK_SIZE);
constexpr static unsigned SURFACE_SKIN_DEPTH = 5;

constexpr static std::size_t CLIMATE_AXES = 5;
constexpr static std::array<const char*, CLIMATE_AXES> AXIS_NAMES {
    "temperature",
    "humidity",
    "continentalness",
    "erosion",
    "weirdness",
};

struct ClimateSample final {
    float temperature;
    float humidity;
    float continentalness;
    float erosion;
    float weirdness;
};

using ClimateSampleArray = std::array<ClimateSample, constant::CHUNK_AREA>;

struct Vox17Settings final {
    float terrain_frequency { 0.005f };
    int terrain_octaves { 4 };

    float terrain_density_peak { 0.385f };
    BlockPos::value_type sea_level { 0 };

    float base_min { -24.0f };
    float base_max { 16.0f };

    float variation_min { 16.0f };
    float variation_max { 96.0f };

    float pv_valley_offset { -32.0f };
    float pv_peak_offset { 12.0f };

    std::array<float, CLIMATE_AXES> climate_frequencies { 0.0007f, 0.0007f, 0.00035f, 0.00075f, 0.001f };
};

struct Vox17State final {
    Vox17Settings settings;
    std::once_flag seed_once;

    std::unique_ptr<NoiseCache3D_4x8x4> terrain_noise;
    std::unique_ptr<NoiseCache2D_8x8> climate_temperature;
    std::unique_ptr<NoiseCache2D_8x8> climate_humidity;
    std::unique_ptr<NoiseCache2D_8x8> climate_continentalness;
    std::unique_ptr<NoiseCache2D_8x8> climate_erosion;
    std::unique_ptr<NoiseCache2D_8x8> climate_weirdness;
};

static fnl_state make_climate_noise(int seed, float frequency)
{
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.frequency = frequency;
    noise.seed = seed;

    return noise;
}

// The world seed appears only after universe::init, so the noise starts on first use
static void seed_state(Vox17State* state, std::uint64_t seed)
{
    std::call_once(state->seed_once, [state, seed] {
        std::mt19937_64 terrain_seeder;
        terrain_seeder.seed(seed);

        auto noise_terrain = fnlCreateState();
        noise_terrain.seed = static_cast<int>(terrain_seeder());
        noise_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
        noise_terrain.fractal_type = FNL_FRACTAL_FBM;
        noise_terrain.frequency = state->settings.terrain_frequency;
        noise_terrain.octaves = state->settings.terrain_octaves;

        state->terrain_noise = std::make_unique<NoiseCache3D_4x8x4>(std::move(noise_terrain));

        std::mt19937_64 climate_seeder;
        climate_seeder.seed(seed);

        auto make_noise = [&](std::size_t axis) {
            auto axis_seed = static_cast<int>(climate_seeder());
            return make_climate_noise(axis_seed, state->settings.climate_frequencies[axis]);
        };

        state->climate_temperature = std::make_unique<NoiseCache2D_8x8>(make_noise(0));
        state->climate_humidity = std::make_unique<NoiseCache2D_8x8>(make_noise(1));
        state->climate_continentalness = std::make_unique<NoiseCache2D_8x8>(make_noise(2));
        state->climate_erosion = std::make_unique<NoiseCache2D_8x8>(make_noise(3));
        state->climate_weirdness = std::make_unique<NoiseCache2D_8x8>(make_noise(4));
    });
}

static ClimateSampleArray sample_climate_array(const Vox17State* state, const ChunkPosXZ& pos)
{
    ClimateSampleArray samples {};

    auto& temperature = state->climate_temperature->get(pos);
    auto& humidity = state->climate_humidity->get(pos);
    auto& continentalness = state->climate_continentalness->get(pos);
    auto& erosion = state->climate_erosion->get(pos);
    auto& weirdness = state->climate_weirdness->get(pos);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos = utils::to_local_xz(i);

        samples[i].temperature = NoiseCache2D_8x8::sample(temperature, lpos);
        samples[i].humidity = NoiseCache2D_8x8::sample(humidity, lpos);
        samples[i].continentalness = NoiseCache2D_8x8::sample(continentalness, lpos);
        samples[i].erosion = NoiseCache2D_8x8::sample(erosion, lpos);
        samples[i].weirdness = NoiseCache2D_8x8::sample(weirdness, lpos);
    }

    return samples;
}

static std::array<float, CLIMATE_AXES> sample_to_unit(const ClimateSample& sample)
{
    return {
        utils::normalize_01(sample.temperature),
        utils::normalize_01(sample.humidity),
        utils::normalize_01(sample.continentalness),
        utils::normalize_01(sample.erosion),
        utils::normalize_01(sample.weirdness),
    };
}

static bool is_inside_terrain(float variation_noise, float y_relative, float base_variation)
{
    auto variation = base_variation * (1.0f - variation_noise * variation_noise);
    auto noise = variation * variation_noise - y_relative;
    return noise > 0.0f;
}

static bool is_inside_terrain(const NoiseCache3D_4x8x4::array_type& terrain, const LocalPos& lpos, float y_relative, float base_variation)
{
    auto variation_noise = NoiseCache3D_4x8x4::sample(terrain, lpos);
    return is_inside_terrain(variation_noise, y_relative, base_variation);
}

static bool is_inside_terrain(const Vox17State* state, const BlockPos& bpos, float y_relative, float base_variation)
{
    auto variation_noise = state->terrain_noise->get_slow(bpos);
    return is_inside_terrain(variation_noise, y_relative, base_variation);
}

static bool column_solid_pass_1(const Vox17State* state, BlockPos::value_type wx, BlockPos::value_type y, BlockPos::value_type wz,
    float base, float variation)
{
    auto y_relative = static_cast<float>(y) - base;
    auto density_range = variation * state->settings.terrain_density_peak;

    if(y_relative > density_range) {
        return false;
    }

    if(y_relative < -density_range) {
        return true;
    }

    auto bpos = BlockPos(wx, y, wz);

    return is_inside_terrain(state, bpos, y_relative, variation);
}

static BlockPos::value_type column_liquid_y(const Vox17State* state, BlockPos::value_type wx, BlockPos::value_type surface_y,
    BlockPos::value_type wz, float base, float variation)
{
    auto liquid_y = Column::UNSET;

    for(auto y = surface_y + 1; y < state->settings.sea_level; ++y) {
        if(column_solid_pass_1(state, wx, y, wz, base, variation)) {
            break;
        }

        liquid_y = y;
    }

    return liquid_y;
}

static Column probe_one_column(const Vox17State* state, BlockPos::value_type bx, BlockPos::value_type bz, float base, float variation)
{
    for(auto y = constant::SURFACE_MAX_Y; y >= constant::SURFACE_MIN_Y; --y) {
        if(column_solid_pass_1(state, bx, y, bz, base, variation)) {
            Column column {};
            column.surface_y = y;
            column.liquid_y = column_liquid_y(state, bx, y, bz, base, variation);
            return column;
        }
    }

    return {};
}

template<>
std::shared_ptr<void> Generator<Vox17>::create_state(const JSON_Object* settings, const WorldDefinition& definition, std::string_view where)
{
    Vox17Settings values;
    Vox17Settings defaults;

    if(settings) {
        if(auto value = utils::parse_arithmetic<float>(settings, "terrain_frequency")) {
            values.terrain_frequency = value.value();
        }

        if(auto value = utils::parse_integer<long long>(settings, "terrain_octaves")) {
            values.terrain_octaves = static_cast<int>(value.value());
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "terrain_density_peak")) {
            values.terrain_density_peak = value.value();
        }

        if(auto value = utils::parse_integer<long long>(settings, "sea_level")) {
            values.sea_level = static_cast<BlockPos::value_type>(value.value());
        }

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

        if(auto value = utils::parse_arithmetic<float>(settings, "pv_valley_offset")) {
            values.pv_valley_offset = value.value();
        }

        if(auto value = utils::parse_arithmetic<float>(settings, "pv_peak_offset")) {
            values.pv_peak_offset = value.value();
        }

        if(auto climate = json_object_get_object(settings, "climate_frequencies")) {
            for(std::size_t i = 0; i < CLIMATE_AXES; ++i) {
                if(auto value = utils::parse_arithmetic<float>(climate, AXIS_NAMES[i])) {
                    values.climate_frequencies[i] = value.value();
                }
            }
        }
    }

    if(!std::isfinite(values.terrain_frequency) || values.terrain_frequency <= 0.0f) {
        msg::log_warning("{}: terrain_frequency is invalid; the default is used", where);
        values.terrain_frequency = defaults.terrain_frequency;
    }

    if(values.terrain_octaves < 1 || values.terrain_octaves > 8) {
        msg::log_warning("{}: terrain_octaves is invalid; the default is used", where);
        values.terrain_octaves = defaults.terrain_octaves;
    }

    if(!std::isfinite(values.terrain_density_peak) || values.terrain_density_peak < 0.0f || values.terrain_density_peak > 1.0f) {
        msg::log_warning("{}: terrain_density_peak is invalid; the default is used", where);
        values.terrain_density_peak = defaults.terrain_density_peak;
    }

    if(values.sea_level < definition.min_y || values.sea_level > definition.max_y) {
        msg::log_warning("{}: sea_level is outside the world height; it is clamped", where);
        values.sea_level = std::clamp(values.sea_level, definition.min_y, definition.max_y);
    }

    if(!std::isfinite(values.base_min) || !std::isfinite(values.base_max) || values.base_min > values.base_max) {
        msg::log_warning("{}: base_min and base_max are invalid; the defaults are used", where);
        values.base_min = defaults.base_min;
        values.base_max = defaults.base_max;
    }

    auto variation_valid = std::isfinite(values.variation_min) && std::isfinite(values.variation_max);
    auto variation_ordered = values.variation_min >= 0.0f && values.variation_min <= values.variation_max;

    if(!variation_valid || !variation_ordered) {
        msg::log_warning("{}: variation_min and variation_max are invalid; the defaults are used", where);
        values.variation_min = defaults.variation_min;
        values.variation_max = defaults.variation_max;
    }

    auto pv_valid = std::isfinite(values.pv_valley_offset) && std::isfinite(values.pv_peak_offset);
    auto pv_ordered = values.pv_valley_offset <= values.pv_peak_offset;

    if(!pv_valid || !pv_ordered) {
        msg::log_warning("{}: pv_valley_offset and pv_peak_offset are invalid; the defaults are used", where);
        values.pv_valley_offset = defaults.pv_valley_offset;
        values.pv_peak_offset = defaults.pv_peak_offset;
    }

    for(std::size_t i = 0; i < CLIMATE_AXES; ++i) {
        if(!std::isfinite(values.climate_frequencies[i]) || values.climate_frequencies[i] <= 0.0f) {
            msg::log_warning("{}: climate_frequencies.{} is invalid; the default is used", where, AXIS_NAMES[i]);
            values.climate_frequencies[i] = defaults.climate_frequencies[i];
        }
    }

    auto& selector = definition.biome_selector;

    if(selector.strategy == BIOME_SELECTION_NEAREST) {
        if(selector.axes.size() != CLIMATE_AXES) {
            msg::log_error("{}: vox17 needs five climate axes; the world is skipped", where);
            return nullptr;
        }

        for(std::size_t i = 0; i < CLIMATE_AXES; ++i) {
            if(selector.axes[i] != AXIS_NAMES[i]) {
                msg::log_error("{}: vox17 expects the {} axis at position {}; the world is skipped", where, AXIS_NAMES[i], i);
                return nullptr;
            }
        }
    }
    else if(selector.strategy == BIOME_SELECTION_GRID) {
        if(selector.axes.size() != 2) {
            msg::log_error("{}: vox17 needs two climate axes; the world is skipped", where);
            return nullptr;
        }

        if(selector.axes[0] != "temperature") {
            msg::log_error("{}: vox17 expects the temperature axis first; the world is skipped", where);
            return nullptr;
        }

        if(selector.axes[1] != "humidity") {
            msg::log_error("{}: vox17 expects the humidity axis second; the world is skipped", where);
            return nullptr;
        }
    }
    else {
        msg::log_error("{}: vox17 needs a nearest or grid biome selector; the world is skipped", where);
        return nullptr;
    }

    auto state = std::make_shared<Vox17State>();
    state->settings = values;

    return state;
}

template<>
ColumnSlice Generator<Vox17>::sample_heightmap(ServerWorld* world, const ChunkPosXZ& pos)
{
    auto state = static_cast<Vox17State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    thread_local std::array<float, constant::CHUNK_AREA> variation_array;
    thread_local std::array<float, constant::CHUNK_AREA> base_array;

    variation_array.fill(state->settings.variation_min);
    base_array.fill(state->settings.base_min);

    ColumnSlice slice {};
    auto climate_array = sample_climate_array(state, pos);
    auto origin_x = static_cast<BlockPos::value_type>(pos.x()) << constant::CHUNK_SIZE_LOG2;
    auto origin_z = static_cast<BlockPos::value_type>(pos.y()) << constant::CHUNK_SIZE_LOG2;

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];

        auto continentalness = utils::normalize_01(sample.continentalness);
        auto erosion = utils::normalize_01(sample.erosion);
        auto pv = utils::peaks_valleys(sample.weirdness);

        variation_array[i] = std::lerp(state->settings.variation_max, state->settings.variation_min, erosion);
        base_array[i] = std::lerp(state->settings.base_min, state->settings.base_max, continentalness);
        base_array[i] += std::lerp(state->settings.pv_valley_offset, state->settings.pv_peak_offset, utils::normalize_01(pv));
    }

    for(LocalPos::value_type lz = 0; lz < constant::CHUNK_SIZE; lz += 1) {
        for(LocalPos::value_type lx = 0; lx < constant::CHUNK_SIZE; lx += 1) {
            auto index = static_cast<std::size_t>(lx + lz * constant::CHUNK_SIZE);
            auto column = probe_one_column(state, origin_x + lx, origin_z + lz, base_array[index], variation_array[index]);
            slice[index] = std::move(column);
        }
    }

    return slice;
}

template<>
void Generator<Vox17>::generate_biomes(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out)
{
    auto state = static_cast<Vox17State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto& definition = world->definition();
    auto& selector = definition.biome_selector;
    auto climate_array = sample_climate_array(state, pos);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto unit = sample_to_unit(climate_array[i]);

        out[i] = selector.select(unit);
    }
}

template<>
bool Generator<Vox17>::generate_blocks(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes)
{
    auto& definition = world->definition();
    auto min_y = definition.min_y >> constant::CHUNK_SIZE_LOG2;
    auto max_y = definition.max_y >> constant::CHUNK_SIZE_LOG2;

    if(pos.y() < min_y || pos.y() > max_y) {
        return false;
    }

    auto state = static_cast<Vox17State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto& selector = definition.biome_selector;

    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_basic_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_filler_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_surface_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_fluid_array;
    thread_local std::array<float, constant::CHUNK_AREA> variation_array;
    thread_local std::array<float, constant::CHUNK_AREA> base_array;

    palette_basic_array.fill(BLOCK_ID_NULL);
    palette_filler_array.fill(BLOCK_ID_NULL);
    palette_surface_array.fill(BLOCK_ID_NULL);
    palette_fluid_array.fill(BLOCK_ID_NULL);
    variation_array.fill(state->settings.variation_min);
    base_array.fill(state->settings.base_min);

    auto cpos_xz = ChunkPosXZ(pos.x(), pos.z());
    auto climate_array = sample_climate_array(state, cpos_xz);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];
        auto unit = sample_to_unit(sample);
        auto biome = selector.select(unit);
        auto biome_def = biome_registry::find_definition(biome);

        if(biome && biome_def) {
            palette_basic_array[i] = biome_def->palette_basic.cached;
            palette_filler_array[i] = biome_def->palette_filler.cached;
            palette_surface_array[i] = biome_def->palette_surface.cached;
            palette_fluid_array[i] = biome_def->palette_fluid.cached;
            biomes[i] = biome;
        }

        auto continentalness = utils::normalize_01(sample.continentalness);
        auto erosion = utils::normalize_01(sample.erosion);
        auto pv = utils::peaks_valleys(sample.weirdness);

        variation_array[i] = std::lerp(state->settings.variation_max, state->settings.variation_min, erosion);
        base_array[i] = std::lerp(state->settings.base_min, state->settings.base_max, continentalness);
        base_array[i] += std::lerp(state->settings.pv_valley_offset, state->settings.pv_peak_offset, utils::normalize_01(pv));
    }

    auto& terrain_array = state->terrain_noise->get(pos);

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);

        auto base = base_array[index_xz];
        auto variation = variation_array[index_xz];
        auto y_relative = static_cast<float>(bpos.y()) - base;
        auto density_range = variation * state->settings.terrain_density_peak;

        auto solid = false;

        if(y_relative < -density_range) {
            solid = true;
        }
        else if(y_relative <= density_range) {
            solid = is_inside_terrain(terrain_array, lpos, y_relative, variation);
        }

        if(solid) {
            blocks.set(i, palette_basic_array[index_xz]);
        }
        else if(bpos.y() < state->settings.sea_level) {
            blocks.set(i, palette_fluid_array[index_xz]);
        }
    }

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);

        auto base = base_array[index_xz];
        auto variation = variation_array[index_xz];
        auto y_relative = static_cast<float>(bpos.y()) - base;
        auto density_range = variation * state->settings.terrain_density_peak;

        if(y_relative > density_range) {
            continue;
        }

        auto current = blocks.get(i);

        if(current == BLOCK_ID_NULL || current == palette_fluid_array[index_xz]) {
            continue;
        }

        auto depth = 0U;
        auto underwater = false;

        for(unsigned dy = 0; dy < SURFACE_SKIN_DEPTH; dy += 1) {
            auto d_lpos = LocalPos(lpos.x(), lpos.y() + static_cast<LocalPos::value_type>(dy + 1), lpos.z());
            auto d_bpos = utils::to_block(pos, d_lpos);
            auto d_index = utils::to_index(d_lpos);

            if(d_lpos.y() >= CHUNK_SIZE_LP) {
                auto y_relative_above = static_cast<float>(d_bpos.y()) - base;

                if(!is_inside_terrain(state, d_bpos, y_relative_above, variation)) {
                    underwater = dy == 0 && d_bpos.y() < state->settings.sea_level;
                    break;
                }

                depth += 1U;
            }
            else {
                auto above = blocks.get(d_index);

                if(above == BLOCK_ID_NULL || above == palette_fluid_array[index_xz]) {
                    underwater = dy == 0 && above == palette_fluid_array[index_xz];
                    break;
                }

                depth += 1U;
            }
        }

        if(depth < SURFACE_SKIN_DEPTH) {
            if(underwater || depth > 0) {
                blocks.set(i, palette_filler_array[index_xz]);
            }
            else {
                blocks.set(i, palette_surface_array[index_xz]);
            }
        }
    }

    world->commit_placements(blocks, pos);

    return true;
}

void Vox17::register_generator(void)
{
    generator_map::add<Vox17>("vox17");
}
