#include "server/pch.hh"

#include "server/world/placement_scatter.hh"

#include "shared/constant.hh"
#include "shared/res/feature.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/block_storage.hh"

#include "server/utils/heightmap.hh"
#include "server/utils/rotate.hh"
#include "server/world/generator_map.hh"
#include "server/world/world.hh"

constexpr static std::uint32_t ENTRY_NONE = std::numeric_limits<std::uint32_t>::max();

struct WouldPlaceKey final {
    constexpr bool operator==(const WouldPlaceKey& other) const
    {
        auto result = true;
        result = result && x == other.x;
        result = result && z == other.z;
        result = result && entry_index == other.entry_index;
        return result;
    }

    BlockPos::value_type x;
    BlockPos::value_type z;
    std::size_t entry_index;
};

template<>
struct std::hash<WouldPlaceKey> final {
    constexpr std::size_t operator()(const WouldPlaceKey& key) const
    {
        auto seed = static_cast<std::uint64_t>(key.x) * UINT64_C(0x9E3779B97F4A7C15);
        seed ^= static_cast<std::uint64_t>(key.z) * UINT64_C(0xC2B2AE3D27D4EB4F);
        seed ^= static_cast<std::uint64_t>(key.entry_index) * UINT64_C(0xFF51AFD7ED558CCD);
        seed ^= seed >> 33;
        return static_cast<std::size_t>(seed);
    }
};

struct ColumnWinner final {
    std::uint32_t entry_index { ENTRY_NONE };
    biome_id_type biome { BIOME_ID_NULL };
    BlockPos::value_type surface_y { Column::UNSET };
};

struct ScatterScanCache final {
    vx::hash_map<WouldPlaceKey, bool> would_place;
    vx::hash_map<BlockPosXZ, ColumnWinner> winners;
    vx::hash_map<BlockPosXZ, std::uint64_t> entropy;
    vx::hash_map<ChunkPosXZ, const BiomeSlice*> biomes;
};

static float scatter_roll(std::uint64_t entropy, std::size_t entry_index, const Identifier& feature_id)
{
    auto roll_seed = entropy;
    roll_seed ^= static_cast<std::uint64_t>(entry_index) * UINT64_C(0x9E3779B97F4A7C15);
    roll_seed ^= std::hash<Identifier>()(feature_id);
    roll_seed ^= roll_seed >> 33;
    roll_seed *= UINT64_C(0xFF51AFD7ED558CCD);
    roll_seed ^= roll_seed >> 33;
    return static_cast<float>(roll_seed >> 33) / static_cast<float>(1ULL << 31);
}

static float tie_priority(std::uint64_t entropy)
{
    auto seed = entropy;
    seed ^= UINT64_C(0xD6E8FEB86659FD93);
    seed ^= seed >> 33;
    seed *= UINT64_C(0xFF51AFD7ED558CCD);
    seed ^= seed >> 33;
    seed *= UINT64_C(0xC4CEB9FE1A85EC53);
    seed ^= seed >> 33;
    return static_cast<float>(seed >> 33) / static_cast<float>(1ULL << 31);
}

static block_id_type block_at(const PlacementContext& context, const BlockPos& bpos, block_id_type fallback)
{
    if(context.storage && context.position == utils::to_chunk(bpos)) {
        return context.storage->get(utils::to_local(bpos));
    }

    return fallback;
}

static block_id_type surface_block_at(const PlacementContext& context, const BlockPos& origin, const Column& column,
    const BiomeDefinition* biome)
{
    if(context.storage && context.position == utils::to_chunk(origin)) {
        return context.storage->get(utils::to_local(origin));
    }

    auto generator = context.world->generator();

    if(generator) {
        auto pos = BlockPosXZ(origin.x(), origin.z());
        return generator->surface_block(context.world, pos, column);
    }

    return biome->palette_land_surface.cached;
}

static bool matches_tags(block_id_type block_id, block_tag_bit required)
{
    if(required == BLOCK_TAG_NONE) {
        return true;
    }

    if(block_id == BLOCK_ID_NULL) {
        return static_cast<bool>(required & BLOCK_TAG_GAS);
    }

    return block_registry::has_tag_any(block_id, required);
}

static bool can_place(const PlacementContext& context, const BlockPos& origin, const BiomeDefinition* biome, const BiomeScatterEntry* entry,
    const Column& column)
{
    if(!column.is_valid()) {
        return false;
    }

    if(origin.y() != column.surface_y) {
        return false;
    }

    if(column.liquid_y >= column.surface_y) {
        return false;
    }

    auto above = BlockPos(origin.x(), origin.y() + 1, origin.z());
    auto in_chunk = utils::to_chunk(origin) == context.position;

    auto origin_block = surface_block_at(context, origin, column, biome);
    auto above_block = block_at(context, above, BLOCK_ID_NULL);

    if(origin_block == biome->palette_fluid.cached) {
        return false;
    }

    if(in_chunk && above_block == biome->palette_fluid.cached) {
        return false;
    }

    if(!matches_tags(origin_block, entry->need_below)) {
        return false;
    }

    if(in_chunk && !matches_tags(above_block, entry->need_above)) {
        return false;
    }

    return true;
}

static std::uint64_t entropy_cached(const PlacementContext& context, ScatterScanCache& cache, BlockPos::value_type x,
    BlockPos::value_type z)
{
    BlockPosXZ key;
    key[0] = x;
    key[1] = z;

    auto it = cache.entropy.find(key);

    if(it != cache.entropy.cend()) {
        return it->second;
    }

    auto value = context.world->entropy_at(key);
    cache.entropy.emplace(key, value);
    return value;
}

static biome_id_type biome_cached(const PlacementContext& context, ScatterScanCache& cache, BlockPos::value_type x, BlockPos::value_type z)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(x >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(z >> constant::CHUNK_SIZE_LOG2);

    auto it = cache.biomes.find(cpos);

    if(it == cache.biomes.cend()) {
        auto& slice = context.world->sample_biomes(cpos);
        auto inserted = cache.biomes.emplace(cpos, &slice);
        it = inserted.first;
    }

    auto lx = static_cast<std::size_t>(x & (constant::CHUNK_SIZE - 1));
    auto lz = static_cast<std::size_t>(z & (constant::CHUNK_SIZE - 1));

    return it->second->at(lx + lz * constant::CHUNK_SIZE);
}

static bool check_footprint(const PlacementContext& context, ScatterScanCache& cache, BlockPos::value_type bx, BlockPos::value_type bz,
    const Feature* feature, unsigned edge, const BiomeDefinition* biome)
{
    auto& bounds = feature->bounds;
    auto pad = static_cast<int>(edge);

    auto min_x = bounds.min().x() - pad;
    auto max_x = bounds.max().x() + pad;
    auto min_z = bounds.min().z() - pad;
    auto max_z = bounds.max().z() + pad;

    for(auto dz = min_z; dz <= max_z; dz += 1) {
        for(auto dx = min_x; dx <= max_x; dx += 1) {
            auto biome_id = biome_cached(context, cache, bx + dx, bz + dz);
            auto biome_def = biome_registry::find_definition(biome_id);

            if(biome == biome_def)
                continue;
            return false;
        }
    }

    return true;
}

static bool fits_terrain(const PlacementContext& context, const Feature* feature, const BlockPos& origin, unsigned slope)
{
    auto& bounds = feature->bounds;

    BlockAlignedBoxXZ columns;
    columns.min()[0] = origin.x() + bounds.min().x();
    columns.min()[1] = origin.z() + bounds.min().z();
    columns.max()[0] = origin.x() + bounds.max().x();
    columns.max()[1] = origin.z() + bounds.max().z();

    utils::HeightmapView view(context.world, columns);

    auto width = static_cast<std::size_t>(columns.max().x() - columns.min().x() + 1);
    auto depth = static_cast<std::size_t>(columns.max().y() - columns.min().y() + 1);

    thread_local std::vector<BlockPos::value_type> surfaces;
    surfaces.assign(width * depth, Column::UNSET);

    auto limit = static_cast<BlockPos::value_type>(slope);

    for(std::size_t dz = 0; dz < depth; dz += 1) {
        for(std::size_t dx = 0; dx < width; dx += 1) {
            auto x = columns.min().x() + static_cast<BlockPos::value_type>(dx);
            auto z = columns.min().y() + static_cast<BlockPos::value_type>(dz);

            auto& column = view.at(x, z);

            if(!column.is_valid()) {
                return false;
            }

            auto index = dx + dz * width;
            surfaces[index] = column.surface_y;

            if(limit == 0) {
                continue;
            }

            if(dx > 0) {
                auto step = column.surface_y - surfaces[index - 1];

                if(step > limit || step < -limit) {
                    return false;
                }
            }

            if(dz > 0) {
                auto step = column.surface_y - surfaces[index - width];

                if(step > limit || step < -limit) {
                    return false;
                }
            }
        }
    }

    for(auto& part : feature->parts) {
        auto px = static_cast<std::size_t>(part.offset.x() - bounds.min().x());
        auto pz = static_cast<std::size_t>(part.offset.z() - bounds.min().z());
        auto index = px + pz * width;

        auto surface = surfaces[index];
        auto target = origin.y() + part.offset.y();

        if(part.ground) {
            if(surface != origin.y()) {
                return false;
            }

            continue;
        }

        if(part.soft) {
            continue;
        }

        if(target <= surface) {
            return false;
        }
    }

    return true;
}

static bool would_place_uncached(std::uint64_t entropy, BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context,
    const BiomeDefinition* biome, const BiomeScatterEntry* entry, std::size_t entry_index, ScatterScanCache& cache)
{
    if(entry->cached == nullptr) {
        return false;
    }

    if(scatter_roll(entropy, entry_index, entry->feature) >= entry->chance) {
        return false;
    }

    BlockPosXZ sample_xz;
    sample_xz[0] = bx;
    sample_xz[1] = bz;

    auto& column = utils::heightmap_probe_slow(context.world, sample_xz);
    auto position = BlockPos(bx, column.surface_y, bz);

    if(!can_place(context, position, biome, entry, column)) {
        return false;
    }

    return check_footprint(context, cache, bx, bz, entry->cached.get(), entry->edge, biome);
}

static bool would_place(BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context, const BiomeDefinition* biome,
    const BiomeScatterEntry* entry, std::size_t entry_index, ScatterScanCache& cache)
{
    WouldPlaceKey key {};
    key.x = bx;
    key.z = bz;
    key.entry_index = entry_index;

    auto it = cache.would_place.find(key);

    if(it == cache.would_place.cend()) {
        auto entropy = entropy_cached(context, cache, bx, bz);
        auto result = would_place_uncached(entropy, bx, bz, context, biome, entry, entry_index, cache);
        cache.would_place.emplace(key, result);
        return result;
    }

    return it->second;
}

static ColumnWinner find_column_winner(BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context,
    ScatterScanCache& cache)
{
    BlockPosXZ key;
    key[0] = bx;
    key[1] = bz;

    auto entry = cache.winners.find(key);

    if(entry != cache.winners.cend()) {
        return entry->second;
    }

    ColumnWinner winner {};

    auto biome_id = biome_cached(context, cache, bx, bz);
    auto biome_def = biome_registry::find_definition(biome_id);
    auto& column = utils::heightmap_probe_slow(context.world, key);

    if(biome_def && column.is_valid()) {
        auto origin = BlockPos(bx, column.surface_y, bz);
        auto count = biome_def->scatter.size();

        for(std::size_t index = 0; index < count; index += 1) {
            auto& scatter = biome_def->scatter[index];

            if(!would_place(bx, bz, context, biome_def, &scatter, index, cache)) {
                continue;
            }

            if(!fits_terrain(context, scatter.cached.get(), origin, scatter.slope)) {
                continue;
            }

            winner.entry_index = static_cast<std::uint32_t>(index);
            winner.biome = biome_id;
            winner.surface_y = column.surface_y;
        }
    }

    cache.winners.emplace(key, winner);
    return winner;
}

static BlockPos::value_type scatter_extent(const Feature* feature)
{
    auto& bounds = feature->bounds;
    const auto min_xz = Eigen::Vector2i(bounds.min().x(), bounds.min().z());
    const auto max_xz = Eigen::Vector2i(bounds.max().x(), bounds.max().z());

    auto min_abs = min_xz.cwiseAbs();
    auto max_abs = max_xz.cwiseAbs();

    return static_cast<BlockPos::value_type>(std::max(min_abs.maxCoeff(), max_abs.maxCoeff()));
}

static bool interacts(const BiomeScatterEntry* a, const BiomeScatterEntry* b, std::size_t a_index, std::size_t b_index)
{
    if(a->group_hash && b->group_hash) {
        return true;
    }

    return a->group_hash == 0 && b->group_hash == 0 && a_index == b_index;
}

static BlockPos::value_type interaction_spacing(const BiomeScatterEntry* a, const BiomeScatterEntry* b)
{
    auto padding = static_cast<BlockPos::value_type>(std::max(a->padding, b->padding));
    auto a_extent = scatter_extent(a->cached.get());
    auto b_extent = scatter_extent(b->cached.get());

    return padding + std::max(a_extent, b_extent);
}

static BlockPos::value_type interaction_radius(const BiomeDefinition* biome, std::size_t entry_index)
{
    auto entry = &biome->scatter[entry_index];
    auto result = BlockPos::value_type(0);
    auto count = biome->scatter.size();

    for(std::size_t index = 0; index < count; index += 1) {
        auto candidate = &biome->scatter[index];

        if(candidate->cached == nullptr) {
            continue;
        }

        if(!interacts(entry, candidate, entry_index, index)) {
            continue;
        }

        result = std::max(result, interaction_spacing(entry, candidate));
    }

    return result;
}

static bool outranks(BlockPos::value_type ax, BlockPos::value_type az, BlockPos::value_type bx, BlockPos::value_type bz,
    const PlacementContext& context, ScatterScanCache& cache)
{
    auto a = tie_priority(entropy_cached(context, cache, ax, az));
    auto b = tie_priority(entropy_cached(context, cache, bx, bz));

    if(a != b) {
        return a > b;
    }

    return bx < ax || (bx == ax && bz < az);
}

static bool overlaps_higher(const BlockPos& origin, const BiomeDefinition* biome, std::size_t entry_index, const PlacementContext& context,
    ScatterScanCache& cache)
{
    auto entry = &biome->scatter[entry_index];
    auto radius = interaction_radius(biome, entry_index);
    auto origin_x = origin.x();
    auto origin_z = origin.z();

    if(radius == 0) {
        return false;
    }

    for(auto dx = -radius; dx <= radius; dx += 1) {
        for(auto dz = -radius; dz <= radius; dz += 1) {
            if(dx == 0 && dz == 0) {
                continue;
            }

            auto ox = origin_x + dx;
            auto oz = origin_z + dz;

            auto winner = find_column_winner(ox, oz, context, cache);

            if(winner.entry_index == ENTRY_NONE) {
                continue;
            }

            auto neighbour_biome = biome_registry::find_definition(winner.biome);

            if(neighbour_biome == nullptr) {
                continue;
            }

            auto neighbour_entry = &neighbour_biome->scatter[winner.entry_index];

            if(!interacts(entry, neighbour_entry, entry_index, winner.entry_index)) {
                continue;
            }

            auto spacing = interaction_spacing(entry, neighbour_entry);
            auto near_x = dx >= -spacing && dx <= spacing;
            auto near_z = dz >= -spacing && dz <= spacing;

            if(near_x && near_z && outranks(ox, oz, origin_x, origin_z, context, cache)) {
                return true;
            }
        }
    }

    return false;
}

void PlacementScatter::collect(const ChunkPos& pos, const PlacementContext& context, std::vector<FeatureInstance>& out) const
{
    ScatterScanCache cache;

    auto extent = static_cast<BlockPos::value_type>(context.max_extent);
    auto chunk_origin = utils::to_block(pos);
    auto chunk_max_x = chunk_origin.x() + static_cast<BlockPos::value_type>(constant::CHUNK_SIZE - 1);
    auto chunk_max_z = chunk_origin.z() + static_cast<BlockPos::value_type>(constant::CHUNK_SIZE - 1);

    auto search_min_x = chunk_origin.x() - extent;
    auto search_max_x = chunk_max_x + extent;
    auto search_min_z = chunk_origin.z() - extent;
    auto search_max_z = chunk_max_z + extent;

    for(auto wx = search_min_x; wx <= search_max_x; wx += 1) {
        for(auto wz = search_min_z; wz <= search_max_z; wz += 1) {
            auto winner = find_column_winner(wx, wz, context, cache);

            if(winner.entry_index == ENTRY_NONE) {
                continue;
            }

            auto biome_def = biome_registry::find_definition(winner.biome);

            if(biome_def == nullptr) {
                continue;
            }

            auto origin = BlockPos(wx, winner.surface_y, wz);

            if(overlaps_higher(origin, biome_def, winner.entry_index, context, cache)) {
                continue;
            }

            FeatureInstance instance {};
            instance.origin = origin;
            instance.feature = biome_def->scatter[winner.entry_index].cached;
            instance.facing = BLOCK_FACE_NORTH;

            out.push_back(std::move(instance));
        }
    }
}
