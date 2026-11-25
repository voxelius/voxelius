#ifndef F5E99223_7BE3_4C54_9C31_74E34262ED63
#define F5E99223_7BE3_4C54_9C31_74E34262ED63

#include "shared/world/biome.hh"

#include "client/world/block_cache.hh"

class BiomeCache final {
public:
    constexpr static std::int16_t PADDING = BlockCache::PADDING;
    constexpr static std::int16_t CHUNK_SIZE_I16 = BlockCache::CHUNK_SIZE_I16;

    constexpr static std::size_t SIZE = BlockCache::SIZE;
    constexpr static std::size_t AREA = SIZE * SIZE;

    void init(const ChunkPos& cpos);

    biome_id_type get(const LocalPosXZ& lpos) const;
    biome_id_type get(const LocalPos& lpos) const;

private:
    std::array<biome_id_type, AREA> m_biomes;
};

#endif /* F5E99223_7BE3_4C54_9C31_74E34262ED63 */
