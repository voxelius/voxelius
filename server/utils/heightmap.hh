#ifndef D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51
#define D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51

#include "shared/coord.hh"
#include "shared/world/heightmap.hh"

class ServerWorld;

namespace utils
{
class HeightmapView final {
public:
    explicit HeightmapView(ServerWorld* world, const BlockAlignedBoxXZ& bounds);

    const Column& at(BlockPos::value_type x, BlockPos::value_type z) const;

private:
    constexpr static std::size_t MAX_SLICES = 4;

    ServerWorld* m_world;
    std::array<ChunkPosXZ, MAX_SLICES> m_positions {};
    std::array<const ColumnSlice*, MAX_SLICES> m_slices {};
    std::size_t m_count { 0 };
};
} // namespace utils

namespace utils
{
const Column& heightmap_probe_slow(ServerWorld* world, const BlockPosXZ& pos);
const ColumnSlice& heightmap_probe(ServerWorld* world, const ChunkPosXZ& pos);
} // namespace utils

#endif /* D2E4F6A8_1C3B_4D5E_9F7A_2B4C6D8E1F51 */
