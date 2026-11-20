#include "server/pch.hh"

#include "server/world/realm_surface.hh"

#include "core/identifier.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"

#include "server/world/climate.hh"
#include "server/world/climate_noise.hh"
#include "server/world/feature_placer.hh"
#include "server/world/heightmap.hh"
#include "server/world/noise_cache_3D.hh"
#include "server/world/terrain.hh"

constexpr static float VARIATION_MIN = 16.0f;
constexpr static float VARIATION_MAX = 96.0f;

constexpr static float BASE_MIN = -24.0f;
constexpr static float BASE_MAX = 16.0f;

constexpr static float PV_VALLEY_OFFSET = -32.0f;
constexpr static float PV_PEAK_OFFSET = 12.0f;
constexpr static float TERRAIN_DENSITY_PEAK = 0.385f;

constexpr static BlockPos::value_type SEA_LEVEL = 0;
constexpr static LocalPos::value_type CHUNK_SIZE_LP = static_cast<LocalPos::value_type>(constant::CHUNK_SIZE);

static std::unique_ptr<NoiseCache3D_4x8x4> s_terrain;

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

static bool is_inside_terrain(const BlockPos& bpos, float y_relative, float base_variation)
{
    auto variation_noise = s_terrain->get_slow(bpos);
    return is_inside_terrain(variation_noise, y_relative, base_variation);
}

static bool column_solid_pass_1(BlockPos::value_type wx, BlockPos::value_type y, BlockPos::value_type wz, float base, float variation)
{
    auto y_relative = static_cast<float>(y) - base;
    auto density_range = variation * TERRAIN_DENSITY_PEAK;

    if(y_relative > density_range) {
        return false;
    }

    if(y_relative < -density_range) {
        return true;
    }

    auto bpos = BlockPos(wx, y, wz);

    return is_inside_terrain(bpos, y_relative, variation);
}

static BlockPos::value_type column_liquid_y(BlockPos::value_type wx, BlockPos::value_type surface_y, BlockPos::value_type wz, float base,
    float variation)
{
    auto liquid_y = Column::UNSET;

    for(auto y = surface_y + 1; y < SEA_LEVEL; ++y) {
        if(column_solid_pass_1(wx, y, wz, base, variation)) {
            break;
        }

        liquid_y = y;
    }

    return liquid_y;
}

static Column probe_one_column(BlockPos::value_type bx, BlockPos::value_type bz, float base, float variation)
{
    for(auto y = constant::SURFACE_MAX_Y; y >= constant::SURFACE_MIN_Y; y -= 1) {
        if(column_solid_pass_1(bx, y, bz, base, variation)) {
            Column column {};
            column.surface_y = y;
            column.liquid_y = column_liquid_y(bx, y, bz, base, variation);
            return column;
        }
    }

    return {};
}

void realm_surface::init(std::mt19937_64& seeder)
{
    fnl_state noise_terrain = fnlCreateState();
    noise_terrain.seed = static_cast<int>(seeder());
    noise_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    noise_terrain.fractal_type = FNL_FRACTAL_FBM;
    noise_terrain.frequency = 0.005f;
    noise_terrain.octaves = 4;

    s_terrain = std::make_unique<NoiseCache3D_4x8x4>(std::move(noise_terrain));
}

void realm_surface::shutdown(void)
{
    s_terrain.reset();
}

void realm_surface::generate(BlockStorage& blocks, BiomeSlice& biomes, const ChunkPos& pos)
{
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
    variation_array.fill(VARIATION_MIN);
    base_array.fill(BASE_MIN);

    auto cpos_xz = ChunkPosXZ(pos.x(), pos.z());
    auto climate_array = climate_noise::sample_array(cpos_xz);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];
        auto biome = climate::find(BIOME_REALM_SURFACE, sample);
        auto biome_def = biome_registry::find_definition(biome);

        if(biome && biome_def) {
            palette_basic_array[i] = biome_def->palette_basic.cached;
            palette_filler_array[i] = biome_def->palette_filler.cached;
            palette_surface_array[i] = biome_def->palette_surface.cached;
            palette_fluid_array[i] = biome_def->palette_fluid.cached;
            biomes[i] = biome;
        }

        auto continentalness = climate::normalize_01(sample.continentalness);
        auto erosion = climate::normalize_01(sample.erosion);
        auto pv = climate::peaks_valleys(sample.weirdness);

        variation_array[i] = std::lerp(VARIATION_MAX, VARIATION_MIN, erosion);
        base_array[i] = std::lerp(BASE_MIN, BASE_MAX, continentalness);
        base_array[i] += std::lerp(PV_VALLEY_OFFSET, PV_PEAK_OFFSET, climate::normalize_01(pv));
    }

    auto& terrain_array = s_terrain->get(pos);

    //
    // Pass 1 -- density basic terrain fill
    //

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);

        auto base = base_array[index_xz];
        auto variation = variation_array[index_xz];
        auto y_relative = static_cast<float>(bpos.y()) - base;
        auto density_range = variation * TERRAIN_DENSITY_PEAK;

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
        else if(bpos.y() < SEA_LEVEL) {
            blocks.set(i, palette_fluid_array[index_xz]);
        }
    }

    //
    // Pass 2 -- surface skin
    //

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto lpos = utils::to_local(i);
        auto bpos = utils::to_block(pos, lpos);
        auto index_xz = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);

        auto base = base_array[index_xz];
        auto variation = variation_array[index_xz];
        auto y_relative = static_cast<float>(bpos.y()) - base;
        auto density_range = variation * TERRAIN_DENSITY_PEAK;

        if(y_relative > density_range) {
            continue;
        }

        auto current = blocks.get(i);

        if(current == BLOCK_ID_NULL || current == palette_fluid_array[index_xz]) {
            continue;
        }

        auto depth = 0U;
        auto underwater = false;

        for(unsigned dy = 0; dy < 5; dy += 1) {
            auto d_lpos = LocalPos(lpos.x(), lpos.y() + static_cast<LocalPos::value_type>(dy + 1), lpos.z());
            auto d_bpos = utils::to_block(pos, d_lpos);
            auto d_index = utils::to_index(d_lpos);

            if(d_lpos.y() >= CHUNK_SIZE_LP) {
                auto y_relative_above = static_cast<float>(d_bpos.y()) - base;

                if(!is_inside_terrain(d_bpos, y_relative_above, variation)) {
                    underwater = dy == 0 && d_bpos.y() < SEA_LEVEL;
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

        if(depth < 5) {
            if(underwater || depth > 0) {
                blocks.set(i, palette_filler_array[index_xz]);
            }
            else {
                blocks.set(i, palette_surface_array[index_xz]);
            }
        }
    }

    feature_placer::commit(blocks, pos, BIOME_REALM_SURFACE);
}

ColumnSlice realm_surface::probe(const ChunkPosXZ& pos)
{
    thread_local std::array<float, constant::CHUNK_AREA> variation_array;
    thread_local std::array<float, constant::CHUNK_AREA> base_array;

    variation_array.fill(VARIATION_MIN);
    base_array.fill(BASE_MIN);

    ColumnSlice slice {};
    auto climate_array = climate_noise::sample_array(pos);
    auto origin_x = static_cast<BlockPos::value_type>(pos.x()) << constant::CHUNK_SIZE_LOG2;
    auto origin_z = static_cast<BlockPos::value_type>(pos.y()) << constant::CHUNK_SIZE_LOG2;

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        auto& sample = climate_array[i];

        auto continentalness = climate::normalize_01(sample.continentalness);
        auto erosion = climate::normalize_01(sample.erosion);
        auto pv = climate::peaks_valleys(sample.weirdness);

        variation_array[i] = std::lerp(VARIATION_MAX, VARIATION_MIN, erosion);
        base_array[i] = std::lerp(BASE_MIN, BASE_MAX, continentalness);
        base_array[i] += std::lerp(PV_VALLEY_OFFSET, PV_PEAK_OFFSET, climate::normalize_01(pv));
    }

    for(LocalPos::value_type lz = 0; lz < constant::CHUNK_SIZE; lz += 1) {
        for(LocalPos::value_type lx = 0; lx < constant::CHUNK_SIZE; lx += 1) {
            auto index = static_cast<std::size_t>(lx + lz * constant::CHUNK_SIZE);
            auto column = probe_one_column(origin_x + lx, origin_z + lz, base_array[index], variation_array[index]);
            slice[index] = std::move(column);
        }
    }

    return slice;
}
