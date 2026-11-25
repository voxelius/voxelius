#ifndef F2B6C9D4_1A7E_4E83_9B5D_3C8F0A6E2D71
#define F2B6C9D4_1A7E_4E83_9B5D_3C8F0A6E2D71

#include "shared/coord.hh"
#include "shared/world/heightmap.hh"

namespace columns
{
void clear(void);
void prune(const ChunkAlignedBoxXZ& box);
const Column* find(const BlockPosXZ& bpos);
void set(const ChunkPosXZ& cpos, const ColumnSlice& slice);
void update(const BlockPosXZ& bpos, const Column& column);
} // namespace columns

#endif /* F2B6C9D4_1A7E_4E83_9B5D_3C8F0A6E2D71 */
