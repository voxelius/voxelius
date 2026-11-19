#include "server/pch.hh"

#include "server/world/realm_sky.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"

#include "server/world/climate.hh"
#include "server/world/climate_noise.hh"
#include "server/world/feature_placer.hh"
#include "server/world/heightmap.hh"
#include "server/world/noise_cache_3D.hh"
#include "server/world/terrain.hh"

constexpr static float BASE_CENTER_Y = 320.0f;
constexpr static float MAX_Y_BIAS = 96.0f;

// Density falloff makes terrain impossible outside this envelope when abs(noise) <= 1.
// Above: density > 0.35 + relative_y * 0.068                   --> relative_y < 0.65/0.068 ~ 9.56
// Below: density > -0.15 + 0.006 x depth + 0.00004 x depth^2   --> depth < 110.4
constexpr static float BASE_DENSITY_THRESHOLD = 0.35f;
constexpr static float DENSITY_MAX_ABOVE = 10.0f;
constexpr static float DENSITY_MAX_BELOW = 112.0f;

constexpr static float TERRAIN_Y_MIN = BASE_CENTER_Y - DENSITY_MAX_BELOW;
constexpr static float TERRAIN_Y_MAX = BASE_CENTER_Y + MAX_Y_BIAS + DENSITY_MAX_ABOVE;

constexpr static LocalPos::value_type CHUNK_SIZE_LP = static_cast<LocalPos::value_type>(constant::CHUNK_SIZE);

static std::unique_ptr<NoiseCache3D_4x8x4> s_density;

static bool is_inside_sky_terrain(float density, float relative_y, float density_bias)
{
    auto threshold = BASE_DENSITY_THRESHOLD - density_bias;

    if(relative_y > 0.0f) {
        density -= relative_y * 0.06f;
        threshold += relative_y * 0.008f;
    }
    else {
        auto depth = -relative_y;
        density -= depth * 0.006f + depth * depth * 0.00004f;
        threshold -= std::min(depth * 0.0035f, 0.35f);
    }

    return density > threshold;
}

static bool is_inside_sky_terrain(const NoiseCache3D_4x8x4::array_type& density, const LocalPos& lpos, const BlockPos& bpos,
    float continentalness, float density_bias)
{
    auto center_y = BASE_CENTER_Y + MAX_Y_BIAS * continentalness;
    auto relative_y = static_cast<float>(bpos.y()) - center_y;

    if(relative_y > DENSITY_MAX_ABOVE || relative_y < -DENSITY_MAX_BELOW) {
        return false;
    }

    return is_inside_sky_terrain(NoiseCache3D_4x8x4::sample(density, lpos), relative_y, density_bias);
}

static bool is_inside_sky_terrain(const BlockPos& bpos, float continentalness, float density_bias)
{
    auto center_y = BASE_CENTER_Y + MAX_Y_BIAS * continentalness;
    auto relative_y = static_cast<float>(bpos.y()) - center_y;

    if(relative_y > DENSITY_MAX_ABOVE || relative_y < -DENSITY_MAX_BELOW) {
        return false;
    }

    return is_inside_sky_terrain(s_density->get_slow(bpos), relative_y, density_bias);
}

static Column probe_one_column(BlockPos::value_type bx, BlockPos::value_type bz, float continentalness, float density_bias)
{
    for(auto y = constant::SKY_MAX_Y; y >= constant::SKY_MIN_Y; y -= 1) {
        auto bpos = BlockPos(bx, y, bz);

        if(is_inside_sky_terrain(bpos, continentalness, density_bias)) {
            Column column {};
            column.surface_y = y;

            auto bottom = y;

            for(auto by = y - 1; by >= constant::SKY_MIN_Y; by -= 1) {
                auto below = BlockPos(bx, by, bz);

                if(!is_inside_sky_terrain(below, continentalness, density_bias)) {
                    break;
                }

                bottom = by;
            }

            column.bottom_y = bottom;
            return column;
        }
    }

    return {};
}

void realm_sky::init(std::mt19937_64& seeder)
{
    auto density = fnlCreateState();
    density.seed = static_cast<int>(seeder());
    density.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    density.fractal_type = FNL_FRACTAL_FBM;
    density.frequency = 0.005f;
    density.octaves = 4;

    s_density = std::make_unique<NoiseCache3D_4x8x4>(std::move(density));
}

void realm_sky::shutdown(void)
{
    s_density.reset();
}

void realm_sky::generate(BlockStorage& blocks, BiomeSlice& biomes, const ChunkPos& pos)
{
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_basic_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_filler_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_surface_array;
    thread_local std::array<block_id_type, constant::CHUNK_AREA> palette_fluid_array;
    thread_local std::array<float, constant::CHUNK_AREA> continentalness_array;
    thread_local std::array<float, constant::CHUNK_AREA> density_bias_array;

    palette_basic_array.fill(BLOCK_ID_NULL);
    palette_filler_array.fill(BLOCK_ID_NULL);
    palette_surface_array.fill(BLOCK_ID_NULL);
    palette_fluid_array.fill(BLOCK_ID_NULL);
    continentalness_array.fill(0.0f);
    density_bias_array.fill(0.0f);

    auto chunk_y_min = static_cast<float>(pos.y()) * static_cast<float>(constant::CHUNK_SIZE);
    auto chunk_y_max = chunk_y_min + static_cast<float>(constant::CHUNK_SIZE - 1);

    if(chunk_y_max < TERRAIN_Y_MIN || chunk_y_min > TERRAIN_Y_MAX) {
        return;
    }

    auto chunk_xz = ChunkPosXZ(pos.x(), pos.z());
    auto climate_array = climate_noise::sample_array(chunk_xz);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];
        auto biome = climate::find(BIOME_REALM_SKY, sample);
        auto biome_def = biome_registry::find_definition(biome);

        if(biome && biome_def) {
            palette_basic_array[i] = biome_def->palette_basic.cached;
            palette_filler_array[i] = biome_def->palette_filler.cached;
            palette_surface_array[i] = biome_def->palette_surface.cached;
            palette_fluid_array[i] = biome_def->palette_fluid.cached;
            biomes[i] = biome;
        }

        auto cont = climate::normalize_01(sample.continentalness);
        continentalness_array[i] = cont;

        auto pv_norm = climate::normalize_01(climate::peaks_valleys(sample.weirdness));
        auto ocean_factor = std::max(0.0f, 0.3f - cont) / 0.3f;
        auto mountain_factor = std::max(0.0f, pv_norm - 0.65f) / 0.35f;
        density_bias_array[i] = 0.08f * ocean_factor + 0.12f * mountain_factor;
    }

    auto& density_array = s_density->get(pos);

    //
    // Pass 1 -- density basic terrain fill
    //

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);

        if(is_inside_sky_terrain(density_array, lpos, bpos, continentalness_array[index_xz], density_bias_array[index_xz])) {
            blocks.set(i, palette_basic_array[index_xz]);
        }
    }

    //
    // Pass 2 -- surface skin
    //

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);
        auto current = blocks.get(i);

        if(current == BLOCK_ID_NULL || current == palette_fluid_array[index_xz]) {
            continue;
        }

        auto depth = 0U;

        for(unsigned dy = 0; dy < 5; dy += 1) {
            auto d_lpos = LocalPos(lpos.x(), lpos.y() + static_cast<LocalPos::value_type>(dy + 1), lpos.z());
            auto d_bpos = utils::to_block(pos, d_lpos);
            auto d_index = utils::to_index(d_lpos);

            if(d_lpos.y() >= CHUNK_SIZE_LP) {
                if(!is_inside_sky_terrain(d_bpos, continentalness_array[index_xz], density_bias_array[index_xz])) {
                    break;
                }

                depth += 1U;
            }
            else {
                auto above = blocks.get(d_index);

                if(above == BLOCK_ID_NULL || above == palette_fluid_array[index_xz]) {
                    break;
                }

                depth += 1U;
            }
        }

        if(depth < 5) {
            if(depth > 0) {
                blocks.set(i, palette_filler_array[index_xz]);
            }
            else {
                blocks.set(i, palette_surface_array[index_xz]);
            }
        }
    }

    feature_placer::commit(blocks, pos, BIOME_REALM_SKY);
}

ColumnSlice realm_sky::probe(const ChunkPosXZ& pos)
{
    thread_local std::array<float, constant::CHUNK_AREA> continentalness_array;
    thread_local std::array<float, constant::CHUNK_AREA> density_bias_array;

    continentalness_array.fill(0.0f);
    density_bias_array.fill(0.0f);

    ColumnSlice slice {};
    auto climate_array = climate_noise::sample_array(pos);
    auto origin_x = static_cast<BlockPos::value_type>(pos.x()) << constant::CHUNK_SIZE_LOG2;
    auto origin_z = static_cast<BlockPos::value_type>(pos.y()) << constant::CHUNK_SIZE_LOG2;

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];

        auto cont = climate::normalize_01(sample.continentalness);
        auto pv_norm = climate::normalize_01(climate::peaks_valleys(sample.weirdness));
        auto ocean_factor = std::max(0.0f, 0.3f - cont) / 0.3f;
        auto mountain_factor = std::max(0.0f, pv_norm - 0.65f) / 0.35f;

        continentalness_array[i] = cont;
        density_bias_array[i] = 0.08f * ocean_factor + 0.12f * mountain_factor;
    }

    for(LocalPos::value_type lz = 0; lz < constant::CHUNK_SIZE; lz += 1) {
        for(LocalPos::value_type lx = 0; lx < constant::CHUNK_SIZE; lx += 1) {
            auto index = static_cast<std::size_t>(lx + lz * constant::CHUNK_SIZE);
            auto column = probe_one_column(origin_x + lx, origin_z + lz, continentalness_array[index], density_bias_array[index]);
            slice[index] = std::move(column);
        }
    }

    return slice;
}

ColumnProbe realm_sky::probe_column(const BlockPosXZ& pos)
{
    ColumnProbe result {};

    auto sample = climate_noise::sample_block(pos);
    auto cont = climate::normalize_01(sample.continentalness);
    auto pv_norm = climate::normalize_01(climate::peaks_valleys(sample.weirdness));
    auto ocean_factor = std::max(0.0f, 0.3f - cont) / 0.3f;
    auto mountain_factor = std::max(0.0f, pv_norm - 0.65f) / 0.35f;
    auto density_bias = 0.08f * ocean_factor + 0.12f * mountain_factor;

    result.column = probe_one_column(pos.x(), pos.y(), cont, density_bias);
    result.biome = climate::find(BIOME_REALM_SKY, sample);

    return result;
}
