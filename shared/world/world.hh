#ifndef AC10DC15_9E3A_4C9E_B8B4_E6C5CAE5BB89
#define AC10DC15_9E3A_4C9E_B8B4_E6C5CAE5BB89

#include "shared/world/biome.hh"
#include "shared/world/chunk.hh"

class ChunkCreateEvent final {
public:
    explicit ChunkCreateEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkRemoveEvent final {
public:
    explicit ChunkRemoveEvent(const ChunkPos& pos, const std::shared_ptr<const Chunk>& chunk);
    constexpr const std::shared_ptr<const Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    std::shared_ptr<const Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkUpdateEvent final {
public:
    explicit ChunkUpdateEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkReadyEvent final {
public:
    explicit ChunkReadyEvent(const ChunkPos& pos, const std::shared_ptr<Chunk>& chunk);
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const ChunkPos& pos(void) const;

private:
    std::shared_ptr<Chunk> m_chunk;
    ChunkPos m_pos;
};

class ChunkLightUpdateEvent final {
public:
    explicit ChunkLightUpdateEvent(const ChunkPos& pos);
    constexpr const ChunkPos& pos(void) const;

private:
    ChunkPos m_pos;
};

class BlockUpdateEvent final {
public:
    explicit BlockUpdateEvent(const BlockPos& pos, block_id_type id, const std::shared_ptr<Chunk>& chunk);
    constexpr const std::shared_ptr<Chunk>& chunk(void) const;
    constexpr const BlockPos& bpos(void) const;
    constexpr const ChunkPos& cpos(void) const;
    constexpr const LocalPos& lpos(void) const;
    constexpr block_id_type id(void) const;

private:
    std::shared_ptr<Chunk> m_chunk;
    block_id_type m_id;
    BlockPos m_bpos;
    ChunkPos m_cpos;
    LocalPos m_lpos;
};

namespace world
{
extern vx::hash_map<ChunkPos, std::shared_ptr<Chunk>> chunk_map;
extern entt::registry chunk_registry;
} // namespace world

namespace world
{
std::shared_ptr<Chunk> create_chunk(const ChunkPos& pos);
std::shared_ptr<Chunk> find_chunk(const ChunkPos& pos);
std::shared_ptr<Chunk> find_chunk(entt::entity entity);
} // namespace world

namespace world
{
void remove_chunk(const std::shared_ptr<const Chunk>& chunk);
void remove_chunk(const ChunkPos& pos);
void remove_chunk(entt::entity entity);
} // namespace world

namespace world
{
block_id_type get_block(const ChunkPos& cpos, const LocalPos& lpos);
block_id_type get_block(const BlockPos& pos);
} // namespace world

namespace world
{
fluid_id_type get_fluid(const ChunkPos& cpos, const LocalPos& lpos);
fluid_id_type get_fluid(const BlockPos& pos);
} // namespace world

namespace world
{
std::uint32_t get_fluid_faces(const ChunkPos& cpos, const LocalPos& lpos);
std::uint32_t get_fluid_faces(const BlockPos& pos);
} // namespace world

namespace world
{
fluid_id_type get_state_fluid(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value);
fluid_id_type get_state_fluid(const BlockPos& pos, std::string_view state, std::string_view value);
} // namespace world

namespace world
{
bool set_block(const ChunkPos& cpos, const LocalPos& lpos, block_id_type id);
bool set_block(const BlockPos& pos, block_id_type id);
} // namespace world

namespace world
{
block_light_type get_light(const ChunkPos& cpos, const LocalPos& lpos);
block_light_type get_light(const BlockPos& pos);
} // namespace world

namespace world
{
std::optional<std::string_view> get_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state);
std::optional<std::string_view> get_state(const BlockPos& pos, std::string_view state);
} // namespace world

namespace world
{
bool set_state(const ChunkPos& cpos, const LocalPos& lpos, std::string_view state, std::string_view value);
bool set_state(const BlockPos& pos, std::string_view state, std::string_view value);
} // namespace world

namespace world
{
std::int32_t get_temperature_base(const ChunkPos& cpos, const LocalPos& lpos);
std::int32_t get_temperature_base(const BlockPos& pos);
} // namespace world

namespace world
{
std::int32_t get_temperature(const ChunkPos& cpos, const LocalPos& lpos);
std::int32_t get_temperature(const BlockPos& pos);
} // namespace world

namespace world
{
biome_id_type get_biome(const ChunkPos& cpos, const LocalPos& lpos);
biome_id_type get_biome(const BlockPos& pos);
} // namespace world

namespace world
{
void schedule(const ChunkPos& cpos, const LocalPos& lpos, std::uint64_t deadline, block_tick_source source);
void schedule(const BlockPos& pos, std::uint64_t deadline, block_tick_source source);
} // namespace world

namespace world
{
void fixed_update(void);
} // namespace world

namespace world
{
void purge(void);
} // namespace world

constexpr const std::shared_ptr<Chunk>& ChunkCreateEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkCreateEvent::pos(void) const
{
    return m_pos;
}

constexpr const std::shared_ptr<const Chunk>& ChunkRemoveEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkRemoveEvent::pos(void) const
{
    return m_pos;
}

constexpr const std::shared_ptr<Chunk>& ChunkUpdateEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkUpdateEvent::pos(void) const
{
    return m_pos;
}

constexpr const std::shared_ptr<Chunk>& ChunkReadyEvent::chunk(void) const
{
    return m_chunk;
}

constexpr const ChunkPos& ChunkReadyEvent::pos(void) const
{
    return m_pos;
}

constexpr const ChunkPos& ChunkLightUpdateEvent::pos(void) const
{
    return m_pos;
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
