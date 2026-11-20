#include "server/pch.hh"

#include "server/world/world.hh"

static void generate_entropy(std::uint64_t seed, const ChunkPosXZ& pos, CachedEntropy& data)
{
    std::mt19937_64 random;
    random.seed(seed ^ std::hash<ChunkPosXZ>()(pos));

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        data[i] = random();
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

void ServerWorld::init_generation(std::shared_ptr<void> state)
{
    m_generator_state = std::move(state);

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

    climate::rebuild(this);
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

biome_id_type ServerWorld::biome_at(const BlockPosXZ& pos)
{
    if(auto cached = find_biome(pos)) {
        return cached.value();
    }

    auto sample = sample_climate_block(pos);
    auto biome = climate::find(this, sample);

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
