#ifndef D24E2B2D_5409_45F7_9188_5C8668D39A37
#define D24E2B2D_5409_45F7_9188_5C8668D39A37

#include "shared/coord.hh"

class World;

struct ChunkDirtyMarker final {};

namespace chunk_loader
{
void init(void);
void shutdown(void);
void fixed_update(void);
} // namespace chunk_loader

namespace chunk_loader
{
void request(const std::shared_ptr<World>& world, const ChunkPos& pos);
bool pending(World* world, const ChunkPos& pos);
} // namespace chunk_loader

namespace chunk_loader
{
bool save(const std::shared_ptr<World>& world, const ChunkPos& pos);
} // namespace chunk_loader

#endif /* D24E2B2D_5409_45F7_9188_5C8668D39A37 */
