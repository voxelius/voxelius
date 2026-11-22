#ifndef B0F02F57_26DF_495B_8E6D_A132AAC33F58
#define B0F02F57_26DF_495B_8E6D_A132AAC33F58

#include "shared/coord.hh"
#include "shared/world/biome.hh"
#include "shared/world/biome_slice.hh"

class BiomeMap final {
public:
    BiomeMap(void);

    void purge(void);

    void insert(const ChunkPosXZ& pos, BiomeSlice biomes);
    void attach(entt::registry& registry, entt::entity entity, const ChunkPos& pos);

    biome_id_type get_slow(const BlockPos& pos) const;

    const BiomeSlice& get(const ChunkPos& pos) const;
    const BiomeSlice& get(const ChunkPosXZ& pos) const;

private:
    struct Entry final {
        std::shared_ptr<BiomeSlice> ptr;
        bool initialized { false };
    };

    mutable std::mutex m_mutex;
    vx::hash_map<ChunkPosXZ, Entry> m_slices;
    std::shared_ptr<BiomeSlice> m_void;
};

#endif /* B0F02F57_26DF_495B_8E6D_A132AAC33F58 */
