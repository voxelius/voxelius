#include "server/pch.hh"

#include "server/world/vox2_generator.hh"

#include "core/utils/interpolation.hh"
#include "core/utils/yaml.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/biome_selector.hh"
#include "shared/world/heightmap.hh"

#include "server/utils/climate.hh"
#include "server/utils/heightmap.hh"
#include "server/world/generator_map.hh"
#include "server/world/noise_cache_2d.hh"
#include "server/world/world.hh"

constexpr static int AXIS_COUNT = 2;
constexpr static int CELL_SIZE = 4;
constexpr static int VERTICAL_RESOLUTION = 8;
constexpr static int GRID_SIZE = static_cast<int>(constant::CHUNK_SIZE) / CELL_SIZE + 1;
constexpr static int MAX_OCTAVES = 16;
constexpr static int INTERPOLATION_OCTAVES = 8;
constexpr static int RELIEF_RADIUS = 3;

constexpr static BlockPos::value_type BEACH_RADIUS = 4;
constexpr static BlockPos::value_type MAX_WORLD_HEIGHT = 4096; // Keeps a column of 25 x 513 floats under 64 KiB

constexpr static float MIN_BIOME_DEPTH = -1.8f;
constexpr static float MOUNTAIN_EDGE = 0.15f;

constexpr static float NOISE_WEIGHT_SCALE = 10.0f;
constexpr static float WEIGHT_OFFSET = 0.2f;
constexpr static float WEIGHT_DEPTH_BIAS = 2.0f;
constexpr static float DEPTH_TERM_SHIFT = 0.125f;
constexpr static float DEPTH_TERM_SCALE = 0.265625f;
constexpr static float SCALE_TERM_FACTOR = 96.0f;
constexpr static float SCALE_TERM_SHIFT = 0.9f;
constexpr static float SCALE_TERM_BIAS = 0.1f;
constexpr static float NOISE_FREQUENCY = 684.412f;
constexpr static float NOISE_STRETCH_XZ = 80.0f;
constexpr static float NOISE_STRETCH_Y = 160.0f;
constexpr static float NOISE_PRECISION = 3.3554432E7f;
constexpr static float REFERENCE_SEA_LEVEL = 63.0f;
constexpr static float REFERENCE_HEIGHT = 256.0f;

constexpr static std::uint64_t SALT_BIOMES = UINT64_C(0xA54FF53A5F1D36F1);
constexpr static std::uint64_t SALT_DENSITY = UINT64_C(0x6A09E667F3BCC909);
constexpr static std::uint64_t SALT_STREAM = UINT64_C(0x9E3779B97F4A7C15);

constexpr static std::array<const char*, AXIS_COUNT> AXIS_NAMES {
    "temperature",
    "humidity",
};

static_assert(constant::CHUNK_SIZE % CELL_SIZE == 0);

struct Vox2BiomeSample final {
    float temperature { 0.5f };
    float humidity { 0.5f };
    float continent { 0.0f };
    float erosion { 0.5f };
    float ridge { 0.0f };
    bool ocean { false };
    bool deep_ocean { false };
    bool river { false };
    bool beach { false };
};

struct Vox2Settings final {
    BlockPos::value_type min_y { -128 };
    BlockPos::value_type max_y { 127 };
    BlockPos::value_type sea_level { 0 };
    BlockPos::value_type surface_depth { 4 };
    BlockPos::value_type snow_line { 48 };
    float snow_temperature { 0.2f };
    float density_factor { 1.0f };
    float density_offset { -0.46875f };
    float noise_amplitude { 1.0f };
    float ocean_level { -0.18f };
    float deep_ocean_level { -0.45f };
    float river_width { 0.03f };
    float ocean_depth { -1.0f };
    float deep_ocean_depth { -1.8f };
    float ocean_scale { 0.1f };
    float river_depth { -0.5f };
    float river_scale { 0.0f };
    float beach_depth { 0.0f };
    float beach_scale { 0.025f };
    float ridge_mountain { 0.55f };
    float ridge_hills { 0.35f };
    float mountain_depth { 1.0f };
    float mountain_scale { 0.5f };
    float hills_depth { 0.45f };
    float hills_scale { 0.3f };
};

struct Vox2SurfaceMaterial final {
    block_id_type top { BLOCK_ID_NULL };
    block_id_type filler { BLOCK_ID_NULL };
    block_id_type base { BLOCK_ID_NULL };
    block_id_type fluid { BLOCK_ID_NULL };
};

struct Vox2State final {
    Vox2Settings settings;
    const BiomeSelector* selector { nullptr };
    std::once_flag seed_once;

    std::unique_ptr<NoiseCache2D_4x4> continent;
    std::unique_ptr<NoiseCache2D_4x4> erosion;
    std::unique_ptr<NoiseCache2D_4x4> region;
    std::unique_ptr<NoiseCache2D_4x4> ridge;
    std::unique_ptr<NoiseCache2D_4x4> river;
    std::unique_ptr<NoiseCache2D_4x4> temperature;
    std::unique_ptr<NoiseCache2D_4x4> humidity;

    std::array<fnl_state, MAX_OCTAVES> lower;
    std::array<fnl_state, MAX_OCTAVES> upper;
    std::array<fnl_state, INTERPOLATION_OCTAVES> interpolation;
};

static fnl_state make_state(std::mt19937_64& seeder, fnl_noise_type type, fnl_fractal_type fractal, float frequency, int octaves)
{
    auto state = fnlCreateState();
    state.seed = static_cast<int>(seeder() & 0x7FFFFFFF);
    state.noise_type = type;
    state.fractal_type = fractal;
    state.frequency = frequency;
    state.octaves = octaves;

    return state;
}

static float maintain_precision(float value)
{
    return value - std::floor(value / NOISE_PRECISION + 0.5f) * NOISE_PRECISION;
}

static void seed_field(std::span<fnl_state> states, std::uint64_t seed, int stream)
{
    std::mt19937_64 random(seed ^ (static_cast<std::uint64_t>(stream) * SALT_STREAM));

    for(auto& state : states) {
        state = fnlCreateState();
        state.seed = static_cast<int>(random() & 0x7FFFFFFF);
        state.noise_type = FNL_NOISE_PERLIN;
        state.fractal_type = FNL_FRACTAL_NONE;
        state.frequency = 1.0f;
        state.octaves = 1;
    }
}

static float field_sample(std::span<fnl_state> states, int octave, float x, float y, float z)
{
    auto index = static_cast<std::size_t>(octave);
    auto& state = states[index];
    auto sample_x = static_cast<FNLfloat>(x);
    auto sample_y = static_cast<FNLfloat>(y);
    auto sample_z = static_cast<FNLfloat>(z);

    return fnlGetNoise3D(&state, sample_x, sample_y, sample_z);
}

static void apply_relief(const Vox2Settings& settings, const Vox2BiomeSample& sample, float& depth, float& scale)
{
    auto hill = utils::smoothstep(settings.ridge_hills * 0.5f, settings.ridge_mountain, sample.ridge);
    auto mountain = utils::smoothstep(settings.ridge_mountain, settings.ridge_mountain + MOUNTAIN_EDGE, sample.ridge);

    depth = std::lerp(depth, settings.hills_depth, hill);
    scale = std::lerp(scale, settings.hills_scale, hill);
    depth = std::lerp(depth, settings.mountain_depth, mountain);
    scale = std::lerp(scale, settings.mountain_scale, mountain);
}

static BlockPos::value_type parse_integer_or(ryml::ConstNodeRef settings, std::string_view key, BlockPos::value_type fallback,
    std::string_view where)
{
    auto node = utils::find_child(settings, key);

    if(node.invalid()) {
        return fallback;
    }

    auto value = utils::parse_integer<long long>(node);

    if(!value.has_value()) {
        msg::log_warning("{}: {} is invalid; the default is used", where, key);
        return fallback;
    }

    return static_cast<BlockPos::value_type>(value.value());
}

static float parse_float_or(ryml::ConstNodeRef settings, std::string_view key, float fallback, std::string_view where)
{
    auto node = utils::find_child(settings, key);

    if(node.invalid()) {
        return fallback;
    }

    auto value = utils::parse_arithmetic<float>(node);

    if(!value.has_value()) {
        msg::log_warning("{}: {} is invalid; the default is used", where, key);
        return fallback;
    }

    return value.value();
}

static block_id_type pick(block_id_type value, block_id_type fallback)
{
    if(value == BLOCK_ID_NULL) {
        return fallback;
    }

    return value;
}

static biome_id_type select_biome(const BiomeSelector* selector, const Vox2BiomeSample& sample)
{
    assert(selector);
    assert(selector->axes.size() <= AXIS_COUNT);

    std::array<float, AXIS_COUNT> values {};

    for(std::size_t i = 0; i < selector->axes.size(); ++i) {
        if(selector->axes[i] == AXIS_NAMES[0]) {
            values[i] = sample.temperature;
        }
        else {
            values[i] = sample.humidity;
        }
    }

    return selector->select(std::span<const float>(values.data(), selector->axes.size()));
}

static Vox2SurfaceMaterial resolve_surface(biome_id_type biome, const Vox2BiomeSample& sample, BlockPos::value_type surface_y,
    const Vox2Settings& settings)
{
    Vox2SurfaceMaterial result {};
    auto definition = biome_registry::find_definition(biome);
    auto shore_surface = BLOCK_ID_NULL;
    auto shore_filler = BLOCK_ID_NULL;

    if(definition) {
        result.top = definition->palette_land_surface.cached;
        result.filler = definition->palette_land_filler.cached;
        result.base = definition->palette_base.cached;
        result.fluid = definition->palette_fluid.cached;
        shore_surface = definition->palette_shore_surface.cached;
        shore_filler = definition->palette_shore_filler.cached;
    }

    result.base = pick(result.base, result.filler);
    result.filler = pick(result.filler, result.top);
    result.top = pick(result.top, result.base);

    shore_surface = pick(shore_surface, result.top);
    shore_filler = pick(shore_filler, result.filler);

    if(sample.ocean) {
        if(sample.deep_ocean) {
            result.top = result.base;
        }
        else {
            result.top = shore_surface;
        }

        result.filler = shore_filler;
    }
    else if(sample.river) {
        result.top = shore_filler;
        result.filler = shore_filler;
    }
    else if(sample.beach) {
        result.top = shore_surface;
        result.filler = shore_filler;
    }
    else if(sample.ridge >= settings.ridge_mountain) {
        result.top = result.base;
        result.filler = result.base;
    }

    if(surface_y != Column::UNSET && (surface_y >= settings.snow_line || sample.temperature < settings.snow_temperature)) {
        auto snow = BLOCK_ID_NULL;

        if(definition) {
            snow = definition->palette_snow_surface.cached;
        }

        result.top = pick(snow, result.top);
    }

    if(surface_y != Column::UNSET && surface_y < settings.sea_level) {
        result.top = shore_filler;
    }

    return result;
}

static bool is_known_axis(std::string_view name)
{
    for(auto axis : AXIS_NAMES) {
        if(name == axis) {
            return true;
        }
    }

    return false;
}

static void seed_state(Vox2State* state, std::uint64_t seed)
{
    std::call_once(state->seed_once, [state, seed] {
        std::mt19937_64 seeder(seed ^ SALT_BIOMES);

        state->continent = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.0034f, 4));
        state->erosion = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.0027f, 3));
        state->region = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.0012f, 2));
        state->ridge = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2, FNL_FRACTAL_RIDGED, 0.0046f, 4));
        state->river = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.0013f, 3));
        state->temperature = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.00042f, 3));
        state->humidity = std::make_unique<NoiseCache2D_4x4>(make_state(seeder, FNL_NOISE_OPENSIMPLEX2S, FNL_FRACTAL_FBM, 0.00058f, 3));

        auto density_seed = seed ^ SALT_DENSITY;

        seed_field(state->lower, density_seed, 1);
        seed_field(state->upper, density_seed, 2);
        seed_field(state->interpolation, density_seed, 3);
    });
}

static Vox2BiomeSample sample_block(Vox2State* state, BlockPos::value_type block_x, BlockPos::value_type block_z)
{
    auto& settings = state->settings;
    auto pos = BlockPosXZ(block_x, block_z);

    Vox2BiomeSample result {};
    result.continent = state->continent->get_slow(pos);
    result.erosion = utils::normalize_01(state->erosion->get_slow(pos));
    result.ridge = std::clamp(state->ridge->get_slow(pos), 0.0f, 1.0f);
    result.temperature = utils::normalize_01(state->temperature->get_slow(pos));
    result.humidity = utils::normalize_01(state->humidity->get_slow(pos));
    result.ocean = result.continent < settings.ocean_level;
    result.deep_ocean = result.continent < settings.deep_ocean_level;

    if(!result.ocean) {
        auto river = std::abs(state->river->get_slow(pos));
        result.river = river < settings.river_width;
    }

    if(!result.ocean && !result.river) {
        auto west_ocean = state->continent->get_slow(BlockPosXZ(block_x - BEACH_RADIUS, block_z)) < settings.ocean_level;
        auto east_ocean = state->continent->get_slow(BlockPosXZ(block_x + BEACH_RADIUS, block_z)) < settings.ocean_level;
        auto north_ocean = state->continent->get_slow(BlockPosXZ(block_x, block_z - BEACH_RADIUS)) < settings.ocean_level;
        auto south_ocean = state->continent->get_slow(BlockPosXZ(block_x, block_z + BEACH_RADIUS)) < settings.ocean_level;

        auto horizontal = west_ocean || east_ocean;
        auto vertical = north_ocean || south_ocean;

        result.beach = horizontal || vertical;
    }

    return result;
}

static Vox2BiomeSample sample_cells(Vox2State* state, int cell_x, int cell_z)
{
    return sample_block(state, cell_x * CELL_SIZE, cell_z * CELL_SIZE);
}

static float sample_noise(Vox2State* state, float x, float y, float z)
{
    auto lower_sum = 0.0f;
    auto upper_sum = 0.0f;
    auto interpolation_sum = 0.0f;
    auto octave_scale = 1.0f;
    auto scale = NOISE_FREQUENCY;
    auto horizontal_stretch = scale / NOISE_STRETCH_XZ;
    auto vertical_stretch = scale / NOISE_STRETCH_Y;

    for(auto octave = 0; octave < MAX_OCTAVES; octave += 1) {
        auto local_x = maintain_precision(x * scale * octave_scale);
        auto local_y = maintain_precision(y * scale * octave_scale);
        auto local_z = maintain_precision(z * scale * octave_scale);

        lower_sum += field_sample(state->lower, octave, local_x, local_y, local_z) / octave_scale;
        upper_sum += field_sample(state->upper, octave, local_x, local_y, local_z) / octave_scale;

        if(octave < INTERPOLATION_OCTAVES) {
            auto stretched_x = maintain_precision(x * horizontal_stretch * octave_scale);
            auto stretched_y = maintain_precision(y * vertical_stretch * octave_scale);
            auto stretched_z = maintain_precision(z * horizontal_stretch * octave_scale);

            interpolation_sum += field_sample(state->interpolation, octave, stretched_x, stretched_y, stretched_z) / octave_scale;
        }

        octave_scale /= 2.0f;
    }

    auto delta = std::clamp((interpolation_sum / 10.0f + 1.0f) / 2.0f, 0.0f, 1.0f);
    auto value = std::lerp(lower_sum / 512.0f, upper_sum / 512.0f, delta);

    return value * state->settings.noise_amplitude;
}

static std::size_t column_index(int levels, int cell_x, int cell_z, int level)
{
    return static_cast<std::size_t>((cell_z * GRID_SIZE + cell_x) * (levels + 1) + level);
}

static float column_density(std::span<const float> values, const Vox2Settings& settings, int levels, int local_x, BlockPos::value_type y,
    int local_z)
{
    if(y < settings.min_y || y > settings.max_y) {
        return 0.0f;
    }

    auto at = [values, levels](int cell_x, int cell_z, int level) {
        auto index = column_index(levels, cell_x, cell_z, level);

        return values[index];
    };

    auto cell_x = local_x / CELL_SIZE;
    auto cell_z = local_z / CELL_SIZE;
    auto fraction_x = static_cast<float>(local_x % CELL_SIZE) / CELL_SIZE;
    auto fraction_z = static_cast<float>(local_z % CELL_SIZE) / CELL_SIZE;
    auto level = static_cast<int>((y - settings.min_y) / VERTICAL_RESOLUTION);
    auto fraction_y = static_cast<float>((y - settings.min_y) % VERTICAL_RESOLUTION) / VERTICAL_RESOLUTION;

    auto low_a = at(cell_x, cell_z, level) * (1.0f - fraction_x) + at(cell_x + 1, cell_z, level) * fraction_x;
    auto low_b = at(cell_x, cell_z + 1, level) * (1.0f - fraction_x) + at(cell_x + 1, cell_z + 1, level) * fraction_x;
    auto high_a = at(cell_x, cell_z, level + 1) * (1.0f - fraction_x) + at(cell_x + 1, cell_z, level + 1) * fraction_x;
    auto high_b = at(cell_x, cell_z + 1, level + 1) * (1.0f - fraction_x) + at(cell_x + 1, cell_z + 1, level + 1) * fraction_x;

    auto low = low_a * (1.0f - fraction_y) + high_a * fraction_y;
    auto high = low_b * (1.0f - fraction_y) + high_b * fraction_y;

    return low * (1.0f - fraction_z) + high * fraction_z;
}

static ColumnSlice build_column(Vox2State* state, int chunk_x, int chunk_z)
{
    auto& settings = state->settings;
    auto levels = static_cast<int>((settings.max_y - settings.min_y + 1) / VERTICAL_RESOLUTION);
    std::vector<float> values(static_cast<std::size_t>(GRID_SIZE * GRID_SIZE * (levels + 1)));

    // The coarse grid, the relief weights and the density terms reproduce
    // the vanilla surface generator; the numbers are not free to change
    for(auto cell_z = 0; cell_z < GRID_SIZE; cell_z += 1) {
        for(auto cell_x = 0; cell_x < GRID_SIZE; cell_x += 1) {
            auto center_sample = sample_cells(state, chunk_x * CELL_SIZE + cell_x, chunk_z * CELL_SIZE + cell_z);
            auto center_biome = select_biome(state->selector, center_sample);
            auto center_definition = biome_registry::find_definition(center_biome);
            auto center_depth = 0.1f;
            auto center_scale = 0.2f;

            if(center_definition) {
                center_depth = std::max(center_definition->depth, MIN_BIOME_DEPTH);
                center_scale = center_definition->scale;
            }

            if(center_sample.ocean) {
                if(center_sample.deep_ocean) {
                    center_depth = settings.deep_ocean_depth;
                }
                else {
                    center_depth = settings.ocean_depth;
                }
            }
            else if(center_sample.river) {
                center_depth = settings.river_depth;
            }
            else if(center_sample.beach) {
                center_depth = settings.beach_depth;
            }
            else {
                apply_relief(settings, center_sample, center_depth, center_scale);
            }

            auto depth_sum = 0.0f;
            auto scale_sum = 0.0f;
            auto weight_sum = 0.0f;

            for(auto dz = -RELIEF_RADIUS; dz <= RELIEF_RADIUS; dz += 1) {
                for(auto dx = -RELIEF_RADIUS; dx <= RELIEF_RADIUS; dx += 1) {
                    auto sample = sample_cells(state, chunk_x * CELL_SIZE + cell_x + dx, chunk_z * CELL_SIZE + cell_z + dz);
                    auto biome = select_biome(state->selector, sample);
                    auto definition = biome_registry::find_definition(biome);
                    auto depth = 0.1f;
                    auto scale = 0.2f;

                    if(definition) {
                        depth = std::max(definition->depth, MIN_BIOME_DEPTH);
                        scale = definition->scale;
                    }

                    if(sample.ocean) {
                        if(sample.deep_ocean) {
                            depth = settings.deep_ocean_depth;
                        }
                        else {
                            depth = settings.ocean_depth;
                        }

                        scale = settings.ocean_scale;
                    }
                    else if(sample.river) {
                        depth = settings.river_depth;
                        scale = settings.river_scale;
                    }
                    else if(sample.beach) {
                        depth = settings.beach_depth;
                        scale = settings.beach_scale;
                    }
                    else {
                        apply_relief(settings, sample, depth, scale);
                    }

                    auto weight = NOISE_WEIGHT_SCALE / std::sqrt(static_cast<float>(dx * dx + dz * dz) + WEIGHT_OFFSET);
                    weight /= depth + WEIGHT_DEPTH_BIAS;

                    if(depth > center_depth) {
                        weight *= 0.5f;
                    }

                    depth_sum += depth * weight;
                    scale_sum += scale * weight;
                    weight_sum += weight;
                }
            }

            auto average_depth = depth_sum / weight_sum;
            auto average_scale = scale_sum / weight_sum;
            auto depth_term = (average_depth * 0.5f - DEPTH_TERM_SHIFT) * DEPTH_TERM_SCALE;
            auto scale_term = SCALE_TERM_FACTOR / (average_scale * SCALE_TERM_SHIFT + SCALE_TERM_BIAS);
            auto noise_x = static_cast<float>(chunk_x * CELL_SIZE + cell_x);
            auto noise_z = static_cast<float>(chunk_z * CELL_SIZE + cell_z);

            for(auto y_index = 0; y_index <= levels; y_index += 1) {
                auto noise_y = static_cast<float>(y_index);
                auto noise = sample_noise(state, noise_x, noise_y, noise_z);
                auto y = static_cast<float>(settings.min_y + y_index * VERTICAL_RESOLUTION);
                auto virtual_y = y - static_cast<float>(settings.sea_level) + REFERENCE_SEA_LEVEL;
                auto base = 1.0f - 2.0f * virtual_y / REFERENCE_HEIGHT;
                auto density_bias = base * settings.density_factor + settings.density_offset + depth_term;
                auto density = density_bias * scale_term;

                if(density > 0.0f) {
                    noise += density * 4.0f;
                }
                else {
                    noise += density;
                }

                auto index = column_index(levels, cell_x, cell_z, y_index);
                values[index] = noise;
            }
        }
    }

    ColumnSlice columns {};

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos = utils::to_local_xz(i);
        auto surface = Column::UNSET;

        for(auto y = settings.max_y; y >= settings.min_y; --y) {
            if(column_density(values, settings, levels, lpos.x(), y, lpos.y()) > 0.0f) {
                surface = y;
                break;
            }
        }

        auto& column = columns[i];
        column.surface_y = surface;

        if(surface == Column::UNSET || surface < settings.sea_level) {
            column.liquid_y = settings.sea_level;
        }
        else {
            column.liquid_y = Column::UNSET;
        }
    }

    return columns;
}

template<>
std::shared_ptr<void> Generator<Vox2>::create_state(ryml::ConstNodeRef settings, const WorldDefinition& definition, std::string_view where)
{
    auto values = Vox2Settings {};
    auto defaults = Vox2Settings {};

    values.sea_level = parse_integer_or(settings, "sea_level", values.sea_level, where);
    values.surface_depth = parse_integer_or(settings, "surface_depth", values.surface_depth, where);
    values.snow_line = parse_integer_or(settings, "snow_line", values.snow_line, where);
    values.snow_temperature = parse_float_or(settings, "snow_temperature", values.snow_temperature, where);
    values.density_factor = parse_float_or(settings, "density_factor", values.density_factor, where);
    values.density_offset = parse_float_or(settings, "density_offset", values.density_offset, where);
    values.noise_amplitude = parse_float_or(settings, "noise_amplitude", values.noise_amplitude, where);
    values.ocean_level = parse_float_or(settings, "ocean_level", values.ocean_level, where);
    values.deep_ocean_level = parse_float_or(settings, "deep_ocean_level", values.deep_ocean_level, where);
    values.river_width = parse_float_or(settings, "river_width", values.river_width, where);
    values.ocean_depth = parse_float_or(settings, "ocean_depth", values.ocean_depth, where);
    values.deep_ocean_depth = parse_float_or(settings, "deep_ocean_depth", values.deep_ocean_depth, where);
    values.ocean_scale = parse_float_or(settings, "ocean_scale", values.ocean_scale, where);
    values.river_depth = parse_float_or(settings, "river_depth", values.river_depth, where);
    values.river_scale = parse_float_or(settings, "river_scale", values.river_scale, where);
    values.beach_depth = parse_float_or(settings, "beach_depth", values.beach_depth, where);
    values.beach_scale = parse_float_or(settings, "beach_scale", values.beach_scale, where);
    values.ridge_mountain = parse_float_or(settings, "ridge_mountain", values.ridge_mountain, where);
    values.ridge_hills = parse_float_or(settings, "ridge_hills", values.ridge_hills, where);
    values.mountain_depth = parse_float_or(settings, "mountain_depth", values.mountain_depth, where);
    values.mountain_scale = parse_float_or(settings, "mountain_scale", values.mountain_scale, where);
    values.hills_depth = parse_float_or(settings, "hills_depth", values.hills_depth, where);
    values.hills_scale = parse_float_or(settings, "hills_scale", values.hills_scale, where);

    auto height = definition.max_y - definition.min_y + 1;

    if(height <= 0 || height > MAX_WORLD_HEIGHT) {
        msg::log_error("{}: the world height must be at most {} blocks; the world is skipped", where, MAX_WORLD_HEIGHT);
        return nullptr;
    }

    auto min_aligned = definition.min_y % 16 == 0;
    auto max_aligned = (definition.max_y + 1) % 16 == 0;

    if(!min_aligned || !max_aligned) {
        msg::log_error("{}: min_y and max_y must be aligned to 16-block chunks; the world is skipped", where);
        return nullptr;
    }

    if(values.sea_level < definition.min_y || values.sea_level > definition.max_y) {
        msg::log_warning("{}: sea_level is outside the world height; it is clamped", where);
        values.sea_level = std::clamp(values.sea_level, definition.min_y, definition.max_y);
    }

    if(values.surface_depth < 0 || values.surface_depth > 16) {
        msg::log_warning("{}: surface_depth is invalid; the default is used", where);
        values.surface_depth = defaults.surface_depth;
    }

    if(values.snow_line < definition.min_y || values.snow_line > definition.max_y) {
        msg::log_warning("{}: snow_line is outside the world height; it is clamped", where);
        values.snow_line = std::clamp(values.snow_line, definition.min_y, definition.max_y);
    }

    if(!std::isfinite(values.density_factor) || values.density_factor <= 0.0f) {
        msg::log_warning("{}: density_factor is invalid; the default is used", where);
        values.density_factor = defaults.density_factor;
    }

    if(!std::isfinite(values.density_offset)) {
        msg::log_warning("{}: density_offset is invalid; the default is used", where);
        values.density_offset = defaults.density_offset;
    }

    if(!std::isfinite(values.noise_amplitude) || values.noise_amplitude <= 0.0f || values.noise_amplitude > 16.0f) {
        msg::log_warning("{}: noise_amplitude is invalid; the default is used", where);
        values.noise_amplitude = defaults.noise_amplitude;
    }

    if(!std::isfinite(values.snow_temperature)) {
        msg::log_warning("{}: snow_temperature is invalid; the default is used", where);
        values.snow_temperature = defaults.snow_temperature;
    }

    if(values.ocean_level < -1.0f || values.ocean_level > 0.0f || values.deep_ocean_level > values.ocean_level) {
        msg::log_warning("{}: ocean_level/deep_ocean_level are invalid; the defaults are used", where);
        values.ocean_level = defaults.ocean_level;
        values.deep_ocean_level = defaults.deep_ocean_level;
    }

    if(!std::isfinite(values.river_width) || values.river_width <= 0.0f) {
        msg::log_warning("{}: river_width is invalid; the default is used", where);
        values.river_width = defaults.river_width;
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

        if(selector.axes.empty() || selector.axes.size() > static_cast<std::size_t>(AXIS_COUNT)) {
            msg::log_error("{}: vox2 needs one or two biome axes; the world is skipped", where);
            return nullptr;
        }
    }
    else if(selector.strategy == BIOME_SELECTION_GRID) {
        if(selector.axes.size() != static_cast<std::size_t>(AXIS_COUNT)) {
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

    values.min_y = definition.min_y;
    values.max_y = definition.max_y;

    auto state = std::make_shared<Vox2State>();
    state->settings = values;
    state->selector = &definition.biome_selector;

    return state;
}

template<>
void Generator<Vox2>::generate_biomes(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out)
{
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos = utils::to_local_xz(i);
        auto bpos = utils::to_block_xz(pos, lpos);
        auto sample = sample_block(state, bpos.x(), bpos.y());

        out[i] = select_biome(state->selector, sample);
    }
}

template<>
ColumnSlice Generator<Vox2>::sample_heightmap(ServerWorld* world, const ChunkPosXZ& pos)
{
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    return build_column(state, pos.x(), pos.y());
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
    auto& settings = state->settings;

    generate_biomes(world, cpos_xz, biomes);

    auto& columns = utils::heightmap_probe(world, cpos_xz);

    std::array<Vox2SurfaceMaterial, constant::CHUNK_AREA> materials {};
    std::array<BlockPos::value_type, constant::CHUNK_AREA> surfaces {};

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto lpos = utils::to_local_xz(i);
        auto bpos = utils::to_block_xz(cpos_xz, lpos);
        auto surface = columns[i].surface_y;
        auto sample = sample_block(state, bpos.x(), bpos.y());

        surfaces[i] = surface;
        materials[i] = resolve_surface(biomes[i], sample, surface, settings);
    }

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto y = utils::to_block(pos, lpos).y();
        auto column_index = utils::to_index_xz(LocalPosXZ(lpos.x(), lpos.z()));
        auto surface = surfaces[column_index];
        auto& material = materials[column_index];

        if(surface == Column::UNSET || y > surface) {
            if(y > settings.sea_level) {
                continue;
            }

            if(material.fluid != BLOCK_ID_NULL) {
                blocks.set(i, material.fluid);
            }

            continue;
        }

        auto material_id = material.base;

        if(y == surface) {
            material_id = material.top;
        }
        else if(surface - y <= settings.surface_depth) {
            material_id = material.filler;
        }

        blocks.set(i, material_id);
    }

    world->commit_placements(blocks, pos);

    return true;
}

template<>
block_id_type Generator<Vox2>::surface_block(ServerWorld* world, const BlockPosXZ& pos, const Column& column)
{
    auto state = static_cast<Vox2State*>(world->generator_state());

    assert(state);

    seed_state(state, world->seed);

    auto biome = world->biome_at(pos);
    auto sample = sample_block(state, pos.x(), pos.y());
    auto material = resolve_surface(biome, sample, column.surface_y, state->settings);

    return material.top;
}

void Vox2::register_generator(void)
{
    generator_map::add<Vox2>("vox2");
}
