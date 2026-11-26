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

struct ScatterScanCache final {
    vx::hash_map<WouldPlaceKey, bool> would_place;
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
    const BiomeDefinition& biome)
{
    if(context.storage && context.position == utils::to_chunk(origin)) {
        return context.storage->get(utils::to_local(origin));
    }

    auto generator = context.world->generator();

    if(generator) {
        auto pos = BlockPosXZ(origin.x(), origin.z());
        return generator->surface_block(context.world, pos, column);
    }

    return biome.palette_land_surface.cached;
}

static bool matches_tags(block_id_type block_id, block_tag_bit required)
{
    if(required == static_cast<block_tag_bit>(0)) {
        return true;
    }

    if(block_id == BLOCK_ID_NULL) {
        return static_cast<bool>(required & BLOCK_TAG_GAS);
    }

    return block_registry::has_tag_any(block_id, required);
}

static bool can_place(const PlacementContext& context, const BlockPos& origin, const BiomeDefinition& biome, const BiomeScatterEntry& entry,
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

    if(origin_block == biome.palette_fluid.cached) {
        return false;
    }

    if(in_chunk && above_block == biome.palette_fluid.cached) {
        return false;
    }

    if(entry.need_below != static_cast<block_tag_bit>(0) && !matches_tags(origin_block, entry.need_below)) {
        return false;
    }

    if(entry.need_above != static_cast<block_tag_bit>(0)) {
        if(in_chunk && !matches_tags(above_block, entry.need_above)) {
            return false;
        }
    }

    return true;
}

static bool check_footprint(ServerWorld* world, BlockPos::value_type bx, BlockPos::value_type bz, const Feature& feature, unsigned edge,
    const BiomeDefinition* biome)
{
    auto& bounds = feature.bounds;
    auto pad = static_cast<int>(edge);

    auto min_x = bounds.min().x() - pad;
    auto max_x = bounds.max().x() + pad;
    auto min_z = bounds.min().z() - pad;
    auto max_z = bounds.max().z() + pad;

    for(auto dz = min_z; dz <= max_z; dz += 1) {
        for(auto dx = min_x; dx <= max_x; dx += 1) {
            auto biome_id = world->biome_at(BlockPosXZ(bx + dx, bz + dz));
            auto biome_def = biome_registry::find_definition(biome_id);

            if(biome == biome_def)
                continue;
            return false;
        }
    }

    return true;
}

static bool would_place_uncached(BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context,
    const BiomeDefinition& biome, const BiomeScatterEntry& entry, std::size_t entry_index, ScatterScanCache&)
{
    if(entry.cached == nullptr) {
        return false;
    }

    auto entropy = context.world->entropy_at(BlockPosXZ(bx, bz));

    if(scatter_roll(entropy, entry_index, entry.feature) >= entry.chance) {
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

    return check_footprint(context.world, bx, bz, *entry.cached, entry.edge, &biome);
}

static bool would_place(BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context, const BiomeDefinition& biome,
    const BiomeScatterEntry& entry, std::size_t entry_index, ScatterScanCache& cache)
{
    WouldPlaceKey key {};
    key.x = bx;
    key.z = bz;
    key.entry_index = entry_index;

    auto it = cache.would_place.find(key);

    if(it == cache.would_place.cend()) {
        auto result = would_place_uncached(bx, bz, context, biome, entry, entry_index, cache);
        cache.would_place.emplace(key, result);
        return result;
    }

    return it->second;
}

static BlockAlignedBox scatter_bounds(const BlockPos& origin, const Feature& feature, block_face facing)
{
    const auto bounds = utils::rotate_bounds(feature.bounds, facing);

    BlockAlignedBox world;
    world.min() = origin + bounds.min().cast<BlockPos::value_type>();
    world.max() = origin + bounds.max().cast<BlockPos::value_type>();
    return world;
}

static BlockPos::value_type scatter_extent(const Feature& feature)
{
    const auto& bounds = feature.bounds;
    const auto min_xz = Eigen::Vector2i(bounds.min().x(), bounds.min().z());
    const auto max_xz = Eigen::Vector2i(bounds.max().x(), bounds.max().z());

    auto min_abs = min_xz.cwiseAbs();
    auto max_abs = max_xz.cwiseAbs();

    return static_cast<BlockPos::value_type>(std::max(min_abs.maxCoeff(), max_abs.maxCoeff()));
}

static bool bounds_too_close(const BlockAlignedBox& a, const BlockAlignedBox& b, unsigned padding)
{
    BlockAlignedBox expanded = a;
    expanded.min() -= BlockPos::Constant(padding);
    expanded.max() += BlockPos::Constant(padding);
    return expanded.intersects(b);
}

static bool same_group(const BiomeScatterEntry& a, const BiomeScatterEntry& b, std::size_t a_index, std::size_t b_index)
{
    if(a.group_hash) {
        return a.group_hash == b.group_hash;
    }

    return a_index == b_index;
}

static const BiomeScatterEntry* find_group_winner(BlockPos::value_type bx, BlockPos::value_type bz, const PlacementContext& context,
    const BiomeDefinition& biome, const BiomeScatterEntry& entry, std::size_t entry_index, ScatterScanCache& cache)
{
    const BiomeScatterEntry* winner = nullptr;

    for(std::size_t i = 0; i < biome.scatter.size(); ++i) {
        const auto& candidate = biome.scatter[i];

        if(!same_group(entry, candidate, entry_index, i)) {
            continue;
        }

        if(!would_place(bx, bz, context, biome, candidate, i, cache)) {
            continue;
        }

        winner = &candidate;
    }

    return winner;
}

static BlockPos::value_type group_search_radius(const BiomeDefinition& biome, const BiomeScatterEntry& entry, std::size_t entry_index)
{
    auto search = static_cast<BlockPos::value_type>(entry.padding) + scatter_extent(*entry.cached);

    for(std::size_t i = 0; i < biome.scatter.size(); ++i) {
        auto& candidate = biome.scatter[i];

        if(candidate.cached == nullptr) {
            continue;
        }

        if(!same_group(entry, candidate, entry_index, i)) {
            continue;
        }

        auto candidate_search = static_cast<BlockPos::value_type>(candidate.padding);
        candidate_search += scatter_extent(*candidate.cached);
        search = std::max(search, candidate_search);
    }

    return search;
}

static bool resolve_padding_tie(BlockPos::value_type bx, BlockPos::value_type bz, const Column& column, const PlacementContext& context,
    const BiomeDefinition& biome, const BiomeScatterEntry& entry, std::size_t entry_index, ScatterScanCache& cache)
{
    if(entry.cached == nullptr) {
        return false;
    }

    auto& feature = *entry.cached;
    auto origin = BlockPos(bx, column.surface_y, bz);
    auto current_box = scatter_bounds(origin, feature, BLOCK_FACE_NORTH);
    auto search = group_search_radius(biome, entry, entry_index);
    auto current_priority = tie_priority(context.world->entropy_at(BlockPosXZ(bx, bz)));

    for(auto dx = -search; dx <= search; dx += 1) {
        for(auto dz = -search; dz <= search; dz += 1) {
            if(dx == 0 && dz == 0) {
                continue;
            }

            auto ox = bx + dx;
            auto oz = bz + dz;

            auto neighbour_entry = find_group_winner(ox, oz, context, biome, entry, entry_index, cache);

            if(neighbour_entry == nullptr || neighbour_entry->cached == nullptr) {
                continue;
            }

            BlockPosXZ neighbour_xz;
            neighbour_xz[0] = ox;
            neighbour_xz[1] = oz;

            auto& neighbour_column = utils::heightmap_probe_slow(context.world, neighbour_xz);

            if(!neighbour_column.is_valid()) {
                continue;
            }

            auto neighbour_origin = BlockPos(ox, neighbour_column.surface_y, oz);
            auto neighbour_bounds = scatter_bounds(neighbour_origin, *neighbour_entry->cached, BLOCK_FACE_NORTH);

            if(!bounds_too_close(current_box, neighbour_bounds, entry.padding)) {
                continue;
            }

            auto neighbour_priority = tie_priority(context.world->entropy_at(BlockPosXZ(ox, oz)));

            if(neighbour_priority > current_priority) {
                return true;
            }

            auto same_priority = neighbour_priority == current_priority;
            auto earlier_column = ox < bx || (ox == bx && oz < bz);

            if(same_priority && earlier_column) {
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
            auto biome = context.world->biome_at(BlockPosXZ(wx, wz));
            auto biome_def = biome_registry::find_definition(biome);

            if(biome_def == nullptr || biome_def->scatter.empty()) {
                continue;
            }

            BlockPosXZ sample_pos;
            sample_pos.x() = wx;
            sample_pos.y() = wz;

            auto& column = utils::heightmap_probe_slow(context.world, sample_pos);

            if(!column.is_valid()) {
                continue;
            }

            BlockPos origin;
            origin.x() = wx;
            origin.y() = column.surface_y;
            origin.z() = wz;

            std::optional<FeatureInstance> column_winner;

            for(std::size_t i = 0; i < biome_def->scatter.size(); ++i) {
                auto& entry = biome_def->scatter[i];

                if(!would_place(wx, wz, context, *biome_def, entry, i, cache)) {
                    continue;
                }

                if(resolve_padding_tie(wx, wz, column, context, *biome_def, entry, i, cache)) {
                    continue;
                }

                FeatureInstance instance {};
                instance.origin = origin;
                instance.feature = entry.cached;
                instance.facing = BLOCK_FACE_NORTH;
                column_winner = std::move(instance);
            }

            if(column_winner.has_value()) {
                out.push_back(std::move(column_winner.value()));
            }
        }
    }
}
