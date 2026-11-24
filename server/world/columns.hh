#ifndef C4F8A1D6_7E2B_4A90_B3C5_9D1E6F2A8B47
#define C4F8A1D6_7E2B_4A90_B3C5_9D1E6F2A8B47

#include "shared/world/biome_slice.hh"
#include "shared/world/heightmap.hh"

#include "server/world/zcol_file.hh"

class ServerWorld;
class World;

class ColumnUpdateEvent final {
public:
    ColumnUpdateEvent(World* world, const BlockPosXZ& pos, const Column& column);

    constexpr World* world(void) const;
    constexpr const BlockPosXZ& pos(void) const;
    constexpr const Column& column(void) const;

private:
    World* m_world;
    BlockPosXZ m_pos;
    Column m_column;
};

// The 2D heightmap and biomes of a world, persisted
// in slice/R<X><Z>.zcol, read or generated on demand
class WorldColumns final {
public:
    struct Chunk final {
        ColumnSlice columns;
        BiomeSlice biomes;
        bool dirty { false };
    };

    WorldColumns(void) = default;

    WorldColumns(const WorldColumns& other) = delete;
    WorldColumns& operator=(const WorldColumns& other) = delete;

    void init(ServerWorld* world);
    void save(void);

    // Batches the disk writes and the column
    // packets of every edit since the last call
    void flush(void);

    const Chunk& get(const ChunkPosXZ& cpos);
    void touch(const BlockPos& bpos);

private:
    Chunk& ensure(const ChunkPosXZ& cpos);

    bool read(const ChunkPosXZ& cpos, Chunk& chunk);
    void write(const ChunkPosXZ& cpos, const Chunk& chunk);

    ZcolFile& region_file(const ChunkPosXZ& cpos);

    ServerWorld* m_world { nullptr };
    vx::hash_map<ChunkPosXZ, Chunk> m_chunks;
    vx::hash_map<ChunkPosXZ, std::unique_ptr<ZcolFile>> m_regions;
    vx::hash_set<ChunkPosXZ> m_dirty;
    vx::hash_map<BlockPosXZ, Column> m_updates;
};

namespace columns
{
void init(void);
void shutdown(void);
} // namespace columns

constexpr World* ColumnUpdateEvent::world(void) const
{
    return m_world;
}

constexpr const BlockPosXZ& ColumnUpdateEvent::pos(void) const
{
    return m_pos;
}

constexpr const Column& ColumnUpdateEvent::column(void) const
{
    return m_column;
}

#endif /* C4F8A1D6_7E2B_4A90_B3C5_9D1E6F2A8B47 */
