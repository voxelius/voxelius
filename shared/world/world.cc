#include "shared/pch.hh"

#include "shared/world/world.hh"

#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/weather_registry.hh"

constexpr static float WEATHER_COOLING = 2.0f;     // Celsius lost under full precip
constexpr static float WEATHER_SATURATION = 0.98f; // Humidity under full precip

// A precipitating front cools the air and raises the humidity under it.
// The result scales with the local coverage, so a fringe of the front
// moves the climate less than its center does
static float weather_coverage_at(const World* world, float x, float z)
{
    auto& definition = world->definition();
    auto& state = world->weather();

    auto type = state.type_at(definition.weather, x, z, world->weather_tick);
    auto def = weather_registry::find_definition(type);

    if(def == nullptr || def->precip.types.empty()) {
        return 0.0f;
    }

    return state.coverage(definition.weather, x, z, world->weather_tick);
}

// A roof stops the weather: the air under it keeps the base climate
static bool is_exposed(const World* world, const BlockPos& pos)
{
    auto column = world->find_column(BlockPosXZ(pos.x(), pos.z()));

    return column == nullptr || !column->shelters(pos.y());
}

ChunkCreateEvent::ChunkCreateEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk)
    : m_world(world), m_chunk(chunk), m_pos(pos)
{
    assert(world);
}

ChunkRemoveEvent::ChunkRemoveEvent(World* world, const ChunkPos& pos, const std::shared_ptr<const Chunk>& chunk)
    : m_world(world), m_chunk(chunk), m_pos(pos)
{
    assert(world);
}

ChunkUpdateEvent::ChunkUpdateEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk)
    : m_world(world), m_chunk(chunk), m_pos(pos)
{
    assert(world);
}

ChunkReadyEvent::ChunkReadyEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk)
    : m_world(world), m_chunk(chunk), m_pos(pos)
{
    assert(world);
}

ChunkLightUpdateEvent::ChunkLightUpdateEvent(World* world, const ChunkPos& pos) : m_world(world), m_pos(pos)
{
    assert(world);
}

BlockUpdateEvent::BlockUpdateEvent(World* world, const BlockPos& pos, block_id_type id, const std::shared_ptr<Chunk>& chunk)
    : m_world(world), m_chunk(chunk), m_id(id), m_bpos(pos), m_cpos(utils::to_chunk(pos)), m_lpos(utils::to_local(pos))
{
    assert(world);
}

World* World::of(entt::registry& registry)
{
    return registry.ctx().get<World*>();
}

World::World(std::shared_ptr<const WorldDefinition> definition) : m_definition(std::move(definition))
{
    m_registry.ctx().emplace<World*>(this);
    m_chunk_registry.ctx().emplace<World*>(this);

    component_map::connect_all(m_registry);
}

void World::set_root(std::filesystem::path root)
{
    m_root = std::move(root);
    m_chunk_dir = m_root / "chunk";
    m_entity_dir = m_root / "entity";
}

void World::set_definition(std::shared_ptr<const WorldDefinition> definition)
{
    assert(definition);

    m_definition = std::move(definition);
}

void World::load_config(void)
{
    std::ifstream in(m_root / "world.conf", std::ios::in);

    if(in.is_open()) {
        m_config.load(in);
    }
}

void World::save_config(void)
{
    std::ofstream out(m_root / "world.conf", std::ios::binary | std::ios::trunc);

    if(out.is_open()) {
        m_config.save(out);
    }
}

void World::set_weather_seed(std::uint64_t value)
{
    weather_seed = value;
    m_weather.reseed(value);
}

void World::set_column_lookup(ColumnLookup lookup)
{
    m_column_lookup = lookup;
}

const Column* World::find_column(const BlockPosXZ& pos) const
{
    if(m_column_lookup) {
        return m_column_lookup(pos);
    }

    return nullptr;
}

std::shared_ptr<Chunk> World::create_chunk(const ChunkPos& pos)
{
    auto it = m_chunks.find(pos);

    if(it == m_chunks.cend()) {
        auto entity = m_chunk_registry.create();
        auto chunk = std::make_shared<Chunk>(entity);

        auto& component = m_chunk_registry.emplace<ChunkComponent>(entity);
        component.position = pos;
        component.ptr = chunk;

        m_chunks.insert_or_assign(ChunkPos(pos), std::shared_ptr(chunk));

        m_biomes.attach(m_chunk_registry, entity, pos);

        globals::dispatcher.trigger(ChunkCreateEvent(this, pos, chunk));

        return chunk;
    }

    return it->second;
}

std::shared_ptr<Chunk> World::find_chunk(const ChunkPos& pos) const
{
    auto it = m_chunks.find(pos);

    if(it == m_chunks.cend()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<Chunk> World::find_chunk(entt::entity entity) const
{
    if(m_chunk_registry.valid(entity)) {
        auto& component = m_chunk_registry.get<ChunkComponent>(entity);
        return component.ptr;
    }

    return nullptr;
}

void World::remove_chunk(const std::shared_ptr<const Chunk>& chunk)
{
    if(chunk) {
        remove_chunk(chunk->entity());
    }
}

void World::remove_chunk(const ChunkPos& pos)
{
    auto it = m_chunks.find(pos);

    if(it == m_chunks.cend()) {
        return;
    }

    globals::dispatcher.trigger(ChunkRemoveEvent(this, pos, it->second));

    m_chunk_registry.destroy(it->second->entity());
    m_chunks.erase(it);
}

void World::remove_chunk(entt::entity entity)
{
    if(m_chunk_registry.valid(entity)) {
        const auto& component = m_chunk_registry.get<ChunkComponent>(entity);

        globals::dispatcher.trigger(ChunkRemoveEvent(this, component.position, component.ptr));

        m_chunks.erase(component.position);
        m_chunk_registry.destroy(entity);
    }
}

block_id_type World::get_block(const ChunkPos& cpos, const LocalPos& lpos) const
{
    if(auto chunk = find_chunk(cpos)) {
        return chunk->get_block(lpos);
    }

    return BLOCK_ID_NULL;
}

block_id_type World::get_block(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_block(cpos, lpos);
}

fluid_id_type World::get_fluid(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto id = get_block(cpos, lpos);

    if(auto def = block_registry::find_definition(id)) {
        return def->fluid;
    }

    return FLUID_ID_NULL;
}

fluid_id_type World::get_fluid(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_fluid(cpos, lpos);
}

std::uint32_t World::get_fluid_faces(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto id = get_block(cpos, lpos);

    if(auto def = block_registry::find_definition(id)) {
        return def->fluid_faces;
    }

    return 0;
}

std::uint32_t World::get_fluid_faces(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_fluid_faces(cpos, lpos);
}

fluid_id_type World::get_state_fluid(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value) const
{
    auto id = get_block(cpos, lpos);
    auto def = block_registry::find_definition(id);

    if(def == nullptr) {
        return FLUID_ID_NULL;
    }

    auto family = block_registry::find_family(def->family);

    if(family == nullptr) {
        return FLUID_ID_NULL;
    }

    auto state_key = family->state_hash(state);

    if(!family->states.contains(state_key)) {
        return FLUID_ID_NULL;
    }

    auto value_key = family->state_hash(value);

    return block_registry::preview_fluid(id, state_key, value_key);
}

fluid_id_type World::get_state_fluid(const BlockPos& pos, std::string_view state, std::string_view value) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_state_fluid(cpos, lpos, state, value);
}

bool World::set_block(const ChunkPos& cpos, const LocalPos& lpos, block_id_type id)
{
    if(auto chunk = find_chunk(cpos)) {
        if(chunk->get_block(lpos) == id) {
            return true;
        }

        chunk->set_block(lpos, id);

        globals::dispatcher.trigger(BlockUpdateEvent(this, utils::to_block(cpos, lpos), id, chunk));

        return true;
    }

    return false;
}

bool World::set_block(const BlockPos& pos, block_id_type id)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return set_block(cpos, lpos, id);
}

block_light_type World::get_light(const ChunkPos& cpos, const LocalPos& lpos) const
{
    if(auto chunk = find_chunk(cpos)) {
        auto index = utils::to_index(lpos);
        auto sky = chunk->get_light(index, LIGHT_SKY);
        auto block = chunk->get_light(index, LIGHT_BLOCK);
        return std::max(sky, block);
    }

    return BLOCK_LIGHT_MIN;
}

block_light_type World::get_light(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_light(cpos, lpos);
}

std::optional<std::string_view> World::get_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state) const
{
    auto id = get_block(cpos, lpos);

    if(id == BLOCK_ID_NULL) {
        return std::nullopt;
    }

    auto family = block_registry::find_family_of(id);

    if(family == nullptr) {
        return std::nullopt;
    }

    auto key = family->state_hash(state);
    auto decl_it = family->states.find(key);

    if(decl_it == family->states.cend()) {
        return std::nullopt;
    }

    auto id_it = family->id_states.find(id);

    if(id_it == family->id_states.cend()) {
        return family->state_value(decl_it->second.default_value);
    }

    auto val_it = id_it->second.find(key);

    if(val_it == id_it->second.cend()) {
        return family->state_value(decl_it->second.default_value);
    }

    return family->state_value(val_it->second);
}

std::optional<std::string_view> World::get_state(const BlockPos& pos, std::string_view state) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_state(cpos, lpos, state);
}

bool World::set_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value)
{
    auto id = get_block(cpos, lpos);

    if(id == BLOCK_ID_NULL) {
        return false;
    }

    auto family = block_registry::find_family_of(id);

    if(family == nullptr) {
        return false;
    }

    auto key = family->state_hash(state);

    if(0 == family->states.count(key)) {
        return false;
    }

    vx::hash_map<blockstate_key_type, blockstate_val_type> states;

    for(const auto& it : family->states) {
        states.try_emplace(it.first, it.second.default_value);
    }

    auto id_it = family->id_states.find(id);

    if(id_it != family->id_states.cend()) {
        for(const auto& it : id_it->second) {
            states.insert_or_assign(blockstate_key_type(it.first), blockstate_val_type(it.second));
        }
    }

    states.insert_or_assign(key, family->state_hash(value));

    auto new_id = block_registry::resolve_variant(family->stem_id, states);

    return set_block(cpos, lpos, new_id);
}

bool World::set_state(const BlockPos& pos, std::string_view state, std::string_view value)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return set_state(cpos, lpos, state, value);
}

float World::get_temperature_base(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto biome_def = biome_registry::find_definition(biome(cpos, lpos));

    if(biome_def) {
        return biome_def->temperature;
    }

    return 15.0f;
}

float World::get_temperature_base(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_temperature_base(cpos, lpos);
}

float World::get_temperature(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto bpos = utils::to_block(cpos, lpos);

    auto coverage = 0.0f;

    if(is_exposed(this, bpos)) {
        coverage = weather_coverage_at(this, static_cast<float>(bpos.x()), static_cast<float>(bpos.z()));
    }

    return get_temperature_base(cpos, lpos) - WEATHER_COOLING * coverage; // TODO: add the season bias
}

float World::get_temperature(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_temperature(cpos, lpos);
}

float World::get_temperature(const BlockPos& pos, float coverage) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);

    if(!is_exposed(this, pos)) {
        coverage = 0.0f;
    }

    return get_temperature_base(cpos, lpos) - WEATHER_COOLING * coverage; // TODO: add the season bias
}

float World::get_humidity_base(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto biome_def = biome_registry::find_definition(biome(cpos, lpos));

    if(biome_def) {
        return biome_def->humidity;
    }

    return 0.5f;
}

float World::get_humidity_base(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_humidity_base(cpos, lpos);
}

float World::get_humidity(const ChunkPos& cpos, const LocalPos& lpos) const
{
    auto bpos = utils::to_block(cpos, lpos);
    auto base = get_humidity_base(cpos, lpos);

    auto coverage = 0.0f;

    if(is_exposed(this, bpos)) {
        coverage = weather_coverage_at(this, static_cast<float>(bpos.x()), static_cast<float>(bpos.z()));
    }

    return std::clamp(base + (WEATHER_SATURATION - base) * coverage, 0.0f, 1.0f); // TODO: add the season bias
}

float World::get_humidity(const BlockPos& pos) const
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);
    return get_humidity(cpos, lpos);
}

biome_id_type World::biome(const ChunkPos& cpos, const LocalPos& lpos) const
{
    return m_biomes.get_slow(utils::to_block(cpos, lpos));
}

biome_id_type World::biome(const BlockPos& pos) const
{
    return m_biomes.get_slow(pos);
}

void World::insert_biomes(const ChunkPosXZ& pos, BiomeSlice biomes)
{
    m_biomes.insert(pos, std::move(biomes));
}

bool World::has_biomes(const ChunkPosXZ& pos) const
{
    return m_biomes.has(pos);
}

const BiomeSlice& World::biomes(const ChunkPos& pos) const
{
    return m_biomes.get(pos);
}

const BiomeSlice& World::biomes(const ChunkPosXZ& pos) const
{
    return m_biomes.get(pos);
}

biome_id_type World::biome_slow(const BlockPos& pos) const
{
    return m_biomes.get_slow(pos);
}

void World::purge(void)
{
    m_chunks.clear();
    m_chunk_registry.clear();
    m_registry.clear();
    m_biomes.purge();
    m_heightmap.purge();
    m_weather = {};
}
