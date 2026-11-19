#ifndef E2A7C4D9_5B18_4F30_9C6A_1D8B3E5F7A20
#define E2A7C4D9_5B18_4F30_9C6A_1D8B3E5F7A20

#include "shared/coord.hh"
#include "shared/world/chunk_lod.hh"
#include "shared/world/lod_tile.hh"

#include "client/world/lod_quad.hh"

constexpr static std::uint8_t LOD_CHUNK_LEVEL_NONE = 0xFF;

// One far chunk and the quads that draw it. The mesher fills the quads and
// the renderer only concatenates them, so a packet never rebuilds the world.
struct LodChunkEntry final {
    std::shared_ptr<const ChunkLod> data;
    std::vector<LodChunkQuad> instances;
    std::uint8_t level { LOD_CHUNK_LEVEL_NONE };
    std::uint32_t generation { 0 };
    bool dirty { true };
    bool visible { true };
};

struct LodTileEntry final {
    std::shared_ptr<const LodTile> data;
};

namespace lod_manager
{
void init(void);
void shutdown(void);
} // namespace lod_manager

namespace lod_manager
{
void store_chunk(const ChunkPos& cpos, std::shared_ptr<const ChunkLod> lod);
void store_tile(const LodKey& key, std::shared_ptr<const LodTile> tile);
void evict_outside(const ChunkAlignedBox& lod_box);
} // namespace lod_manager

namespace lod_manager
{
bool enabled(void);
bool debug_levels(void);
bool other_realms(void);
std::int32_t range_chunks(void);
} // namespace lod_manager

namespace lod_manager
{
vx::hash_map<ChunkPos, LodChunkEntry>& chunks(void);
std::vector<std::pair<LodKey, std::shared_ptr<const LodTile>>> snapshot_tiles(void);
} // namespace lod_manager

namespace lod_manager
{
// Changes whenever the quads or their owners change,
// so the renderers know when their buffers are stale.
std::uint64_t mesh_revision(void);
void notify_mesh_changed(void);
} // namespace lod_manager

#endif /* E2A7C4D9_5B18_4F30_9C6A_1D8B3E5F7A20 */
