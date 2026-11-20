#include "server/pch.hh"

#include "server/world/world.hh"

#include "shared/constant.hh"
#include "shared/res/feature.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/block_storage.hh"

#include "server/utils/climate.hh"
#include "server/utils/rotate.hh"
#include "server/world/placement_scatter.hh"

constexpr static int CLIMATE_SIZE = 100;
constexpr static std::size_t MAX_NUDGE_STEPS = 1000000;

using ClimateDir = std::array<int, CLIMATE_DIMS>;

constexpr static std::array STEPS { -1, 0, 1 };
constexpr static std::array AXIS_WEIGHTS { 1.0f, 1.0f, 0.75f, 1.0f, 1.5f };
constexpr static std::size_t STEP_COUNT = STEPS.size();
constexpr static std::size_t NEIGHBOUR_DIR_COUNT = STEP_COUNT * STEP_COUNT * STEP_COUNT * STEP_COUNT * STEP_COUNT - 1;

static std::array<ClimateDir, NEIGHBOUR_DIR_COUNT> s_neighbour_dirs;
static std::once_flag s_neighbour_dirs_once;

static void generate_entropy(std::uint64_t seed, const ChunkPosXZ& pos, CachedEntropy& data)
{
    std::mt19937_64 random;
    random.seed(seed ^ std::hash<ChunkPosXZ>()(pos));

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        data[i] = random();
    }
}

static bool neighbour_is_nonzero(int a, int b, int c, int d, int e)
{
    return a || b || c || d || e;
}

static ClimateDir neighbour_dir(int a, int b, int c, int d, int e)
{
    ClimateDir dir {};
    dir[0] = a;
    dir[1] = b;
    dir[2] = c;
    dir[3] = d;
    dir[4] = e;
    return dir;
}

static std::uint64_t pack_pos(const ClimatePos& pos)
{
    std::uint64_t result = 0;
    result |= static_cast<std::uint64_t>(pos[0]) & 0x7F;
    result |= (static_cast<std::uint64_t>(pos[1]) & 0x7F) << 7;
    result |= (static_cast<std::uint64_t>(pos[2]) & 0x7F) << 14;
    result |= (static_cast<std::uint64_t>(pos[3]) & 0x7F) << 21;
    result |= (static_cast<std::uint64_t>(pos[4]) & 0x7F) << 28;
    return result;
}

static ClimatePos clamp_pos(ClimatePos pos)
{
    pos[0] = static_cast<std::uint8_t>(std::clamp<int>(pos[0], 0, CLIMATE_SIZE - 1));
    pos[1] = static_cast<std::uint8_t>(std::clamp<int>(pos[1], 0, CLIMATE_SIZE - 1));
    pos[2] = static_cast<std::uint8_t>(std::clamp<int>(pos[2], 0, CLIMATE_SIZE - 1));
    pos[3] = static_cast<std::uint8_t>(std::clamp<int>(pos[3], 0, CLIMATE_SIZE - 1));
    pos[4] = static_cast<std::uint8_t>(std::clamp<int>(pos[4], 0, CLIMATE_SIZE - 1));
    return pos;
}

static std::array<float, CLIMATE_DIMS> target_to_unit(const ClimatePos& pos)
{
    constexpr static auto scale = 1.0f / static_cast<float>(CLIMATE_SIZE - 1);

    std::array<float, CLIMATE_DIMS> result {};
    result[0] = scale * static_cast<float>(pos[0]);
    result[1] = scale * static_cast<float>(pos[1]);
    result[2] = scale * static_cast<float>(pos[2]);
    result[3] = scale * static_cast<float>(pos[3]);
    result[4] = scale * static_cast<float>(pos[4]);
    return result;
}

static std::array<float, CLIMATE_DIMS> sample_to_unit(const ClimateSample& sample)
{
    return {
        utils::normalize_01(sample.temperature),
        utils::normalize_01(sample.humidity),
        utils::normalize_01(sample.continentalness),
        utils::normalize_01(sample.erosion),
        utils::normalize_01(sample.weirdness),
    };
}

static bool nudge(vx::hash_map<std::uint64_t, biome_id_type>& occupied, ClimatePos& pos, biome_id_type id, std::mt19937& random)
{
    auto key = pack_pos(pos);

    if(0 == occupied.count(key)) {
        occupied.emplace(key, id);
        return true;
    }

    std::uniform_int_distribution<std::size_t> dir_dist(0, s_neighbour_dirs.size() - 1);
    auto direction = s_neighbour_dirs.at(dir_dist(random));
    auto current = pos;

    for(std::size_t step = 0; step < MAX_NUDGE_STEPS; step += 1) {
        ClimatePos next {};

        while(true) {
            auto in_bounds = true;

            for(std::size_t i = 0; i < CLIMATE_DIMS; ++i) {
                auto value = static_cast<int>(current[i]) + direction[i];

                if(value < 0 || value >= CLIMATE_SIZE) {
                    in_bounds = false;
                    break;
                }

                next[i] = static_cast<std::uint8_t>(value);
            }

            if(in_bounds) {
                break;
            }

            direction = s_neighbour_dirs.at(dir_dist(random));
        }

        current = next;
        key = pack_pos(current);

        if(0 == occupied.count(key)) {
            occupied.emplace(key, id);
            pos = current;
            return true;
        }
    }

    return false;
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

ServerWorld::ServerWorld(std::shared_ptr<const WorldDefinition> definition) : World(std::move(definition))
{
    // empty
}

void ServerWorld::set_generator(const GeneratorDefinition* generator)
{
    assert(generator);

    m_generator = generator;
}

biome_id_type ServerWorld::find_climate_biome(const ClimateSample& sample) const
{
    const auto& seeds = m_climate_seeds;

    if(seeds.empty()) {
        return BIOME_ID_NULL;
    }

    auto unit = sample_to_unit(sample);

    auto best_id = BIOME_ID_NULL;
    auto best_score = std::numeric_limits<float>::max();
    auto best_priority = 0U;

    for(const auto& seed : seeds) {
        auto target = target_to_unit(seed.target);
        auto dist_squared = 0.0f;

        for(std::size_t i = 0; i < CLIMATE_DIMS; ++i) {
            auto delta = unit[i] - target[i];
            delta *= AXIS_WEIGHTS[i];

            dist_squared += delta * delta;
        }

        auto score = dist_squared + seed.offset;

        if(score < best_score || (score == best_score && seed.priority > best_priority)) {
            best_score = score;
            best_priority = seed.priority;
            best_id = seed.id;
        }
    }

    return best_id;
}

void ServerWorld::init_generation(std::shared_ptr<void> state)
{
    m_generator_state = std::move(state);

    m_placement_extent = 0;

    for(auto& def : biome_registry::all_definitions()) {
        for(auto& entry : def.scatter) {
            if(entry.cached == nullptr) {
                continue;
            }

            auto& bounds = entry.cached->bounds;
            auto min_xz = Eigen::Vector2i(bounds.min().x(), bounds.min().z());
            auto max_xz = Eigen::Vector2i(bounds.max().x(), bounds.max().z());
            auto horizontal = std::max(min_xz.cwiseAbs().maxCoeff(), max_xz.cwiseAbs().maxCoeff());

            m_placement_extent = std::max(m_placement_extent, entry.padding + entry.edge + static_cast<std::size_t>(horizontal));
        }
    }

    m_placement_rules.clear();
    m_placement_rules.push_back(std::make_unique<PlacementScatter>());

    std::mt19937_64 seeder;
    seeder.seed(seed);

    fnl_state temperature = fnlCreateState();
    temperature.noise_type = FNL_NOISE_OPENSIMPLEX2;
    temperature.frequency = 0.0007f;
    temperature.seed = static_cast<int>(seeder());

    fnl_state humidity = fnlCreateState();
    humidity.noise_type = FNL_NOISE_OPENSIMPLEX2;
    humidity.frequency = 0.0007f;
    humidity.seed = static_cast<int>(seeder());

    fnl_state continentalness = fnlCreateState();
    continentalness.noise_type = FNL_NOISE_OPENSIMPLEX2;
    continentalness.frequency = 0.00035f;
    continentalness.seed = static_cast<int>(seeder());

    fnl_state erosion = fnlCreateState();
    erosion.noise_type = FNL_NOISE_OPENSIMPLEX2;
    erosion.frequency = 0.00075f;
    erosion.seed = static_cast<int>(seeder());

    fnl_state weirdness = fnlCreateState();
    weirdness.noise_type = FNL_NOISE_OPENSIMPLEX2;
    weirdness.frequency = 0.001f;
    weirdness.seed = static_cast<int>(seeder());

    m_climate_temperature = std::make_unique<NoiseCache2D_8x8>(std::move(temperature));
    m_climate_humidity = std::make_unique<NoiseCache2D_8x8>(std::move(humidity));
    m_climate_continentalness = std::make_unique<NoiseCache2D_8x8>(std::move(continentalness));
    m_climate_erosion = std::make_unique<NoiseCache2D_8x8>(std::move(erosion));
    m_climate_weirdness = std::make_unique<NoiseCache2D_8x8>(std::move(weirdness));

    m_entropy_seed = seeder();

    std::call_once(s_neighbour_dirs_once, [] {
        auto combinations = std::views::cartesian_product(STEPS, STEPS, STEPS, STEPS, STEPS);

        auto nonzero = combinations | std::views::filter([](const auto& tuple) {
            return std::apply(neighbour_is_nonzero, tuple);
        });

        auto dirs = nonzero | std::views::transform([](const auto& tuple) {
            return std::apply(neighbour_dir, tuple);
        });

        std::ranges::copy(dirs, s_neighbour_dirs.begin());
    });

    auto& out = climate_seeds();
    auto definitions = biome_registry::all_definitions();

    out.clear();
    out.reserve(definitions.size());

    std::mt19937 random;
    random.seed(UINT32_C(0xB10BEE5E));

    const auto& allowed = definition().biomes;

    for(biome_id_type id = 1; id < definitions.size(); id += 1) {
        if(!allowed.empty() && !std::ranges::contains(allowed, id)) {
            continue;
        }

        auto& def = definitions[id];

        ClimatePos target {};
        target[0] = def.temperature;
        target[1] = def.humidity;
        target[2] = def.continentalness;
        target[3] = def.erosion;
        target[4] = def.weirdness;

        ClimateSeed seed {};
        seed.id = id;
        seed.priority = def.priority;
        seed.offset = def.offset;
        seed.target = clamp_pos(std::move(target));

        out.emplace_back(std::move(seed));
    }

    std::stable_sort(out.begin(), out.end(), [](const ClimateSeed& a, const ClimateSeed& b) {
        return a.priority > b.priority;
    });

    vx::hash_map<std::uint64_t, biome_id_type> occupied;

    for(auto& seed : out) {
        if(!nudge(occupied, seed.target, seed.id, random)) {
            msg::log_warning("failed to place biome {} uniquely", seed.id);
        }
    }
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

    return entropy[lx + lz * constant::CHUNK_SIZE];
}

std::optional<biome_id_type> ServerWorld::find_biome(const BlockPosXZ& pos) const
{
    std::shared_lock lock(m_biome_mutex);

    auto it = m_biome_cache.find(pos);

    if(it == m_biome_cache.end()) {
        return std::nullopt;
    }

    return it->second;
}

void ServerWorld::store_biome(const BlockPosXZ& pos, biome_id_type biome)
{
    std::unique_lock lock(m_biome_mutex);

    m_biome_cache[pos] = biome;
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

biome_id_type ServerWorld::biome_at(const BlockPosXZ& pos)
{
    if(auto cached = find_biome(pos)) {
        return cached.value();
    }

    auto sample = sample_climate_block(pos);
    auto biome = find_climate_biome(sample);

    store_biome(pos, biome);

    return biome;
}

ClimateSample ServerWorld::sample_climate_block(const BlockPosXZ& pos) const
{
    ClimateSample sample {};
    sample.temperature = m_climate_temperature->get_slow(pos);
    sample.humidity = m_climate_humidity->get_slow(pos);
    sample.continentalness = m_climate_continentalness->get_slow(pos);
    sample.erosion = m_climate_erosion->get_slow(pos);
    sample.weirdness = m_climate_weirdness->get_slow(pos);
    return sample;
}

ClimateSampleArray ServerWorld::sample_climate_array(const ChunkPosXZ& pos) const
{
    ClimateSampleArray samples {};

    auto& temperature = m_climate_temperature->get(pos);
    auto& humidity = m_climate_humidity->get(pos);
    auto& continentalness = m_climate_continentalness->get(pos);
    auto& erosion = m_climate_erosion->get(pos);
    auto& weirdness = m_climate_weirdness->get(pos);

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        LocalPosXZ lpos;
        lpos[0] = static_cast<LocalPosXZ::value_type>(i % constant::CHUNK_SIZE);
        lpos[1] = static_cast<LocalPosXZ::value_type>(i / constant::CHUNK_SIZE);

        samples[i].temperature = NoiseCache2D_8x8::sample(temperature, lpos);
        samples[i].humidity = NoiseCache2D_8x8::sample(humidity, lpos);
        samples[i].continentalness = NoiseCache2D_8x8::sample(continentalness, lpos);
        samples[i].erosion = NoiseCache2D_8x8::sample(erosion, lpos);
        samples[i].weirdness = NoiseCache2D_8x8::sample(weirdness, lpos);
    }

    return samples;
}
