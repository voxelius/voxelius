#ifndef EE0E3E20_655A_4D1C_89FB_25C924073EAC
#define EE0E3E20_655A_4D1C_89FB_25C924073EAC

#include "shared/world/block_storage.hh"
#include "shared/world/light_storage.hh"

class Chunk final {
public:
    using schedule_type = std::multimap<std::uint64_t, std::pair<std::size_t, block_tick_source>>;

    struct Snapshot final {
        BlockStorage blocks;
        LightStorage light;
        bool light_valid { false };
        schedule_type scheduled; // deadlines are relative to the tick the snapshot was taken on
    };

    static void encode(const Chunk& chunk, WriteBuffer& buffer);
    static void decode(Chunk& chunk, ReadBuffer& buffer);

    static void encode_dat(const Snapshot& snapshot, WriteBuffer& buffer);
    static void decode_dat(Snapshot& snapshot, ReadBuffer& buffer);

    explicit Chunk(entt::entity entity);

    block_id_type get_block(std::size_t index) const;
    block_id_type get_block(const LocalPos& pos) const;

    void set_block(std::size_t index, block_id_type id);
    void set_block(const LocalPos& pos, block_id_type id);

    block_light_type get_light(std::size_t index, light_channel channel) const;
    block_light_type get_light(const LocalPos& pos, light_channel channel) const;

    void set_light(std::size_t index, light_channel channel, block_light_type value);
    void set_light(LightStorage light);
    void clear_light(void);

    constexpr entt::entity entity(void) const;
    constexpr const BlockStorage& blocks(void) const;
    constexpr const LightStorage& light(void) const;
    constexpr bool has_light(void) const;

    void set_blocks(BlockStorage blocks);

    Snapshot snapshot(std::uint64_t now) const;
    void restore(Snapshot snapshot, std::uint64_t now);

    void schedule(std::size_t index, std::uint64_t deadline, block_tick_source source);
    void pop_due(std::uint64_t now, std::vector<std::pair<std::size_t, block_tick_source>>& out);

private:
    entt::entity m_entity;
    BlockStorage m_blocks;
    LightStorage m_light;
    bool m_light_valid { false };
    schedule_type m_scheduled;
};

struct Chunk_Component final {
    std::shared_ptr<Chunk> ptr;
    ChunkPos position;
};

constexpr entt::entity Chunk::entity(void) const
{
    return m_entity;
}

constexpr const BlockStorage& Chunk::blocks(void) const
{
    return m_blocks;
}

constexpr const LightStorage& Chunk::light(void) const
{
    return m_light;
}

constexpr bool Chunk::has_light(void) const
{
    return m_light_valid;
}

#endif /* EE0E3E20_655A_4D1C_89FB_25C924073EAC */
