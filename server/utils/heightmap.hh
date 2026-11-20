#ifndef D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51
#define D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51

#include "shared/coord.hh"
#include "shared/world/heightmap.hh"

class ServerWorld;

namespace utils
{
const Column& heightmap_probe_slow(ServerWorld* world, const BlockPosXZ& pos);
const ColumnSlice& heightmap_probe(ServerWorld* world, const ChunkPosXZ& pos);
} // namespace utils

#endif /* D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51 */
