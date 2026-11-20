#ifndef AC10DC15_9E3A_4C9E_B8B4_E6C5CAE5BB89
#define AC10DC15_9E3A_4C9E_B8B4_E6C5CAE5BB89

#include "shared/world/biome_map.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world_definition.hh"

class World;

class ChunkCreateEvent final {
public:
    ChunkCreateEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);

    constexpr World* world(void) const;
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    World* m_world;
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkRemoveEvent final {
public:
    ChunkRemoveEvent(World* world, const ChunkPos& pos, const std::shared_ptr<const Chunk>& chunk);

    constexpr World* world(void) const;
    constexpr const std::shared_ptr<const Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    World* m_world;
    std::shared_ptr<const Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkUpdateEvent final {
public:
    ChunkUpdateEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);

    constexpr World* world(void) const;
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    World* m_world;
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkReadyEvent final {
public:
    ChunkReadyEvent(World* world, const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);

    constexpr World* world(void) const;
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    World* m_world;
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkLightUpdateEvent final {
public:
    ChunkLightUpdateEvent(World* world, const ChunkPos& pos);

    constexpr World* world(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    World* m_world;
    ChunkPos m_pos;
};

class BlockUpdateEvent final {
public:
    BlockUpdateEvent(World* world, const BlockPos& pos, block_id_type id, const std::shared_ptr<Chunk>& chunk);

    constexpr World* world(void) const;
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const BlockPos& bpos(void) const;
    constexpr const ChunkPos& cpos(void) const;
    constexpr const LocalPos& lpos(void) const;
    constexpr block_id_type id(void) const;

private:
    World* m_world;
    std::shared_ptr<Chunk> m_chunk;
    block_id_type m_id;
    BlockPos m_bpos;
    ChunkPos m_cpos;
    LocalPos m_lpos;
};

class World final {
public:
    explicit World(std::shared_ptr<const WorldDefinition> definition);

    const WorldDefinition& definition(void) const;
    const Identifier& id(void) const;

    std::uint64_t seed { 0 };

    entt::registry& chunk_registry(void);
    const vx::hash_map<ChunkPos, std::shared_ptr<Chunk>>& chunks(void) const;

    std::array<WeatherRealmState, NUM_BIOME_REALMS>& weather(void);
    const std::array<WeatherRealmState, NUM_BIOME_REALMS>& weather(void) const;

public:
    std::shared_ptr<Chunk> create_chunk(const ChunkPos& pos);
    std::shared_ptr<Chunk> find_chunk(const ChunkPos& pos) const;
    std::shared_ptr<Chunk> find_chunk(entt::entity entity) const;

public:
    void remove_chunk(const std::shared_ptr<const Chunk>& chunk);
    void remove_chunk(const ChunkPos& pos);
    void remove_chunk(entt::entity entity);

public:
    block_id_type get_block(const ChunkPos& cpos, const LocalPos& lpos) const;
    block_id_type get_block(const BlockPos& pos) const;

    fluid_id_type get_fluid(const ChunkPos& cpos, const LocalPos& lpos) const;
    fluid_id_type get_fluid(const BlockPos& pos) const;

    std::uint32_t get_fluid_faces(const ChunkPos& cpos, const LocalPos& lpos) const;
    std::uint32_t get_fluid_faces(const BlockPos& pos) const;

    fluid_id_type get_state_fluid(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value) const;
    fluid_id_type get_state_fluid(const BlockPos& pos, std::string_view state, std::string_view value) const;

    bool set_block(const ChunkPos& cpos, const LocalPos& lpos, block_id_type id);
    bool set_block(const BlockPos& pos, block_id_type id);

    block_light_type get_light(const ChunkPos& cpos, const LocalPos& lpos) const;
    block_light_type get_light(const BlockPos& pos) const;

    std::optional<std::string_view> get_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state) const;
    std::optional<std::string_view> get_state(const BlockPos& pos, std::string_view state) const;

    bool set_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value);
    bool set_state(const BlockPos& pos, std::string_view state, std::string_view value);

    std::int32_t get_temperature_base(const ChunkPos& cpos, const LocalPos& lpos) const;
    std::int32_t get_temperature_base(const BlockPos& pos) const;

    std::int32_t get_temperature(const ChunkPos& cpos, const LocalPos& lpos) const;
    std::int32_t get_temperature(const BlockPos& pos) const;

    biome_id_type biome(const ChunkPos& cpos, const LocalPos& lpos) const;
    biome_id_type biome(const BlockPos& pos) const;

    void insert_biomes(biome_realm realm, const ChunkPosXZ& pos, BiomeSlice biomes);
    const BiomeSlice& biomes(const ChunkPos& pos) const;
    const BiomeSlice& biomes(biome_realm realm, const ChunkPosXZ& pos) const;
    biome_id_type biome_slow(const BlockPos& pos) const;

    void schedule(const ChunkPos& cpos, const LocalPos& lpos, std::uint64_t deadline, block_tick_source source);
    void schedule(const BlockPos& pos, std::uint64_t deadline, block_tick_source source);

public:
    void fixed_update(void);
    void purge(void);

private:
    std::shared_ptr<const WorldDefinition> m_definition;
    vx::hash_map<ChunkPos, std::shared_ptr<Chunk>> m_chunks;
    entt::registry m_chunk_registry;
    BiomeMap m_biomes;
    std::array<WeatherRealmState, NUM_BIOME_REALMS> m_weather {};
};

constexpr World* ChunkCreateEvent::world(void) const
{
    return m_world;
}

constexpr const std::shared_ptr<Chunk>& ChunkCreateEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkCreateEvent::pos(void) const
{
    return m_pos;
}

constexpr World* ChunkRemoveEvent::world(void) const
{
    return m_world;
}

constexpr const std::shared_ptr<const Chunk>& ChunkRemoveEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkRemoveEvent::pos(void) const
{
    return m_pos;
}

constexpr World* ChunkUpdateEvent::world(void) const
{
    return m_world;
}

constexpr const std::shared_ptr<Chunk>& ChunkUpdateEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkUpdateEvent::pos(void) const
{
    return m_pos;
}

constexpr World* ChunkReadyEvent::world(void) const
{
    return m_world;
}

constexpr const std::shared_ptr<Chunk>& ChunkReadyEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkReadyEvent::pos(void) const
{
    return m_pos;
}

constexpr World* ChunkLightUpdateEvent::world(void) const
{
    return m_world;
}

constexpr const ChunkPos& ChunkLightUpdateEvent::pos(void) const
{
    return m_pos;
}

constexpr World* BlockUpdateEvent::world(void) const
{
    return m_world;
}

constexpr const std::shared_ptr<Chunk>& BlockUpdateEvent::chunk(void) const
{
    return m_chunk;
}

constexpr block_id_type BlockUpdateEvent::id(void) const
{
    return m_id;
}

constexpr const BlockPos& BlockUpdateEvent::bpos(void) const
{
    return m_bpos;
}

constexpr const ChunkPos& BlockUpdateEvent::cpos(void) const
{
    return m_cpos;
}

constexpr const LocalPos& BlockUpdateEvent::lpos(void) const
{
    return m_lpos;
}

#endif /* AC10DC15_9E3A_4C9E_B8B4_E6C5CAE5BB89 */
