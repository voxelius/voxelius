#ifndef A9C4E2F7_1B63_4D80_8F52_C3A1B7D9E604
#define A9C4E2F7_1B63_4D80_8F52_C3A1B7D9E604

#include "shared/coord.hh"
#include "shared/world/biome.hh"
#include "shared/world/chunk_lod.hh"

class BlockStorage;

// The far LOD pyramid of a chunk. It is built where the blocks appear, lives
// as long as the chunk does and drops with it, so no cache has to be
// invalidated when a mod or the world changes.
struct ChunkLod_Component final {
    std::shared_ptr<const ChunkLod> ptr;
};

// A pyramid was rebuilt after a block edit and has to replace the one every
// client in range already holds.
class ChunkLodUpdatedEvent final {
public:
    explicit ChunkLodUpdatedEvent(const ChunkPos& pos);
    constexpr const ChunkPos& pos(void) const;

private:
    ChunkPos m_pos;
};

namespace chunk_lod
{
void init(void);
void shutdown(void);
void fixed_update(void);
} // namespace chunk_lod

namespace chunk_lod
{
// Builds the pyramid of a chunk from its blocks and the biome slice of its
// column. An empty slice means the biomes are unknown and the colours come
// out untinted. Returns false when the chunk holds nothing solid.
bool build(const BlockStorage& blocks, std::span<const biome_id_type> biomes, ChunkLod& lod);
} // namespace chunk_lod

constexpr const ChunkPos& ChunkLodUpdatedEvent::pos(void) const
{
    return m_pos;
}

#endif /* A9C4E2F7_1B63_4D80_8F52_C3A1B7D9E604 */
