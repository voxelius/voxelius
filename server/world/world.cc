#include "server/pch.hh"

#include "server/world/world.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/res/feature.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/world.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/world_scope.hh"

#include "server/utils/rotate.hh"
#include "server/world/generator_map.hh"
#include "server/world/placement_scatter.hh"

static void generate_entropy(std::uint64_t seed, const ChunkPosXZ& pos, CachedEntropy& data)
{
    std::mt19937_64 random;
    random.seed(seed ^ std::hash<ChunkPosXZ>()(pos));

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        data[i] = random();
    }
}

static bool check_intersection(const FeatureInstance& instance, const ChunkPos& cpos)
{
    if(instance.feature == nullptr) {
        return false;
    }

    auto bounds = utils::rotate_bounds(instance.feature->bounds, instance.facing);
    auto chunk_origin = utils::to_block(cpos);

    Eigen::AlignedBox3i chunk_bounds;
    chunk_bounds.min() = Eigen::Vector3i::Zero();
    chunk_bounds.max() = Eigen::Vector3i::Constant(constant::CHUNK_SIZE - 1);

    BlockAlignedBox block_bounds;
    block_bounds.min() = instance.origin + bounds.min().cast<BlockPos::value_type>();
    block_bounds.max() = instance.origin + bounds.max().cast<BlockPos::value_type>();

    auto local_min = block_bounds.min() - chunk_origin;
    auto local_max = block_bounds.max() - chunk_origin;

    Eigen::AlignedBox3i local_bounds;
    local_bounds.min() = local_min.cast<Eigen::Vector3i::Scalar>();
    local_bounds.max() = local_max.cast<Eigen::Vector3i::Scalar>();

    return chunk_bounds.intersects(local_bounds);
}

static void commit_instance(BlockStorage& storage, const ChunkPos& cpos, const FeatureInstance& instance)
{
    if(instance.feature == nullptr) {
        return;
    }

    for(auto& part : instance.feature->parts) {
        auto offset = utils::rotate_offset(part.offset, instance.facing);
        auto bpos = instance.origin + offset.cast<BlockPos::value_type>();

        if(cpos == utils::to_chunk(bpos)) {
            auto lpos = utils::to_local(bpos);
            auto existing = storage.get(lpos);

            if(existing == BLOCK_ID_NULL) {
                storage.set(lpos, part.block);
                continue;
            }

            if(block_registry::has_tag_any(existing, part.overwrite)) {
                storage.set(lpos, part.block);
                continue;
            }
        }
    }
}

ServerWorld::ServerWorld(std::shared_ptr<const ServerWorldDefinition> definition)
    : World(definition), m_server_definition(std::move(definition))
{
    connect_column_lookup<&WorldColumns::find>(m_columns);
}

const CachedEntropy& ServerWorld::sample_entropy(const ChunkPosXZ& pos)
{
    auto entry = get_entropy_entry(pos);

    std::call_once(entry->init_flag, [this, pos, entry] {
        generate_entropy(m_entropy_seed, pos, entry->data);
    });

    return entry->data;
}

std::uint64_t ServerWorld::entropy_at(const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    auto& entropy = sample_entropy(cpos);

    auto lx = static_cast<std::size_t>(pos[0] & (constant::CHUNK_SIZE - 1));
    auto lz = static_cast<std::size_t>(pos[1] & (constant::CHUNK_SIZE - 1));

    auto index = lx + lz * constant::CHUNK_SIZE;

    return entropy[index];
}

biome_id_type ServerWorld::biome_at(const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    auto entry = get_biome_entry(cpos);

    std::call_once(entry->init_flag, [this, cpos, entry] {
        m_server_definition->generator->generate_biomes(this, cpos, entry->data);
    });

    auto lx = static_cast<std::size_t>(pos[0] & (constant::CHUNK_SIZE - 1));
    auto lz = static_cast<std::size_t>(pos[1] & (constant::CHUNK_SIZE - 1));

    auto index = lx + lz * constant::CHUNK_SIZE;

    return entry->data[index];
}

std::shared_ptr<ServerWorld::ScatterEntry> ServerWorld::scatter_entry(const ChunkPosXZ& pos)
{
    if(auto entry = find_scatter(pos)) {
        return entry;
    }

    std::unique_lock lock(m_scatter_mutex);

    auto it = m_scatter_cache.find(pos);

    if(it != m_scatter_cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<ScatterEntry>();
    m_scatter_cache.emplace(pos, entry);

    return entry;
}

void ServerWorld::commit_placements(BlockStorage& storage, const ChunkPos& cpos)
{
    thread_local std::vector<FeatureInstance> candidates;

    candidates.clear();

    auto cpos_xz = ChunkPosXZ(cpos.x(), cpos.z());
    auto& cached = collect_placements(cpos_xz);
    candidates.insert(candidates.end(), cached.cbegin(), cached.cend());

    commit_placements(storage, cpos, candidates);
}

void ServerWorld::commit_placements(BlockStorage& storage, const ChunkPos& cpos, std::span<const FeatureInstance> instances)
{
    for(auto& instance : instances) {
        if(check_intersection(instance, cpos)) {
            commit_instance(storage, cpos, instance);
        }
    }
}

void ServerWorld::init_generation(void)
{
    m_placement_extent = 0;

    for(auto& def : biome_registry::all_definitions()) {
        for(auto& entry : def.scatter) {
            if(entry.cached == nullptr) {
                continue;
            }

            auto& bounds = entry.cached->bounds;
            auto min_xz = Eigen::Vector2i(bounds.min().x(), bounds.min().z());
            auto max_xz = Eigen::Vector2i(bounds.max().x(), bounds.max().z());
            auto min_abs = min_xz.cwiseAbs();
            auto max_abs = max_xz.cwiseAbs();
            auto horizontal = std::max(min_abs.maxCoeff(), max_abs.maxCoeff());

            m_placement_extent = std::max(m_placement_extent, entry.padding + entry.edge + static_cast<std::size_t>(horizontal));
        }
    }

    m_placement_rules.clear();
    m_placement_rules.push_back(std::make_unique<PlacementScatter>());

    // The generator seeds its noise from the first five draws of a seeder
    // keyed by the world seed. Discard them so the entropy seed keeps its
    // old value and feature placement stays stable across this change
    std::mt19937_64 seeder;
    seeder.seed(seed);
    seeder.discard(5);

    m_entropy_seed = seeder();
}

std::shared_ptr<ServerWorld::EntropyEntry> ServerWorld::find_entropy_entry(const ChunkPosXZ& pos) const
{
    std::shared_lock lock(m_entropy_mutex);

    auto it = m_entropy_cache.find(pos);

    if(it == m_entropy_cache.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<ServerWorld::EntropyEntry> ServerWorld::get_entropy_entry(const ChunkPosXZ& pos)
{
    if(auto entry = find_entropy_entry(pos)) {
        return entry;
    }

    std::unique_lock lock(m_entropy_mutex);

    auto it = m_entropy_cache.find(pos);

    if(it != m_entropy_cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<EntropyEntry>();
    m_entropy_cache.emplace(pos, entry);

    return entry;
}

std::shared_ptr<ServerWorld::BiomeEntry> ServerWorld::find_biome_entry(const ChunkPosXZ& pos) const
{
    std::shared_lock lock(m_biome_mutex);

    auto it = m_biome_cache.find(pos);

    if(it == m_biome_cache.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<ServerWorld::BiomeEntry> ServerWorld::get_biome_entry(const ChunkPosXZ& pos)
{
    if(auto entry = find_biome_entry(pos)) {
        return entry;
    }

    std::unique_lock lock(m_biome_mutex);

    auto it = m_biome_cache.find(pos);

    if(it != m_biome_cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<BiomeEntry>();
    m_biome_cache.emplace(pos, entry);

    return entry;
}

std::shared_ptr<ServerWorld::ScatterEntry> ServerWorld::find_scatter(const ChunkPosXZ& pos) const
{
    std::shared_lock lock(m_scatter_mutex);

    auto it = m_scatter_cache.find(pos);

    if(it == m_scatter_cache.end()) {
        return nullptr;
    }

    return it->second;
}

const std::vector<FeatureInstance>& ServerWorld::collect_placements(const ChunkPosXZ& pos)
{
    auto entry = scatter_entry(pos);

    std::call_once(entry->init_flag, [this, pos, entry] {
        PlacementContext context {};
        context.world = this;
        context.position = ChunkPos(pos.x(), 0, pos.y());
        context.storage = nullptr;
        context.max_extent = m_placement_extent;

        for(auto& rule : m_placement_rules) {
            rule->collect(context.position, context, entry->data);
        }
    });

    return entry->data;
}

void ServerWorld::schedule(const ChunkPos& cpos, const LocalPos& lpos, std::uint64_t deadline, block_tick_source source)
{
    if(auto chunk = find_chunk(cpos)) {
        auto index = utils::to_index(lpos);

        chunk->schedule(index, deadline, source);
    }
}

void ServerWorld::schedule(const BlockPos& pos, std::uint64_t deadline, block_tick_source source)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    schedule(cpos, lpos, deadline, source);
}

void ServerWorld::fixed_update(void)
{
    WorldScope scope(this);

    std::vector<std::pair<std::size_t, block_tick_source>> due;

    chunk_registry().view<ChunkComponent>().each([&](ChunkComponent& component) {
        due.clear();

        component.ptr->pop_due(globals::current_tick, due);

        for(auto it : due) {
            utils::block_sched_tick(component.position, component.ptr, it.first, it.second);
        }
    });

    m_columns.flush();
}
