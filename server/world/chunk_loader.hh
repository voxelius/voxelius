#ifndef D24E2B2D_5409_45F7_9188_5C8668D39A37
#define D24E2B2D_5409_45F7_9188_5C8668D39A37

#include "shared/coord.hh"

struct ChunkDirtyMarker final {};

namespace chunk_loader
{
void init(void);
void shutdown(void);
void fixed_update(void);
} // namespace chunk_loader

namespace chunk_loader
{
void request(const ChunkPos& pos);
bool pending(const ChunkPos& pos);
} // namespace chunk_loader

namespace chunk_loader
{
bool save(const ChunkPos& pos);
} // namespace chunk_loader

#endif /* D24E2B2D_5409_45F7_9188_5C8668D39A37 */
