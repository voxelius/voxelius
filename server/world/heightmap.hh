#ifndef C1DAD708_A904_4335_AD46_212640B0DC2B
#define C1DAD708_A904_4335_AD46_212640B0DC2B

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/world/biome.hh"

constexpr static BlockPos::value_type HEIGHTMAP_UNSET = std::numeric_limits<BlockPos::value_type>::min();

struct Column final {
    constexpr static BlockPos::value_type UNSET = HEIGHTMAP_UNSET;

    constexpr bool is_valid(void) const;

    BlockPos::value_type surface_y { UNSET }; // top surface block
    BlockPos::value_type bottom_y { UNSET };  // Bottom block of the top solid interval
    BlockPos::value_type liquid_y { UNSET };  // top liquid block (above surface, otherwise UNSET)
};

using ColumnSlice = std::array<Column, constant::CHUNK_AREA>;

struct ColumnProbe final {
    Column column;
    biome_id_type biome { BIOME_ID_NULL };
};

namespace heightmap
{
void purge(void);
} // namespace heightmap

namespace heightmap
{
const Column& probe_slow(biome_realm realm, const BlockPosXZ& pos);
const ColumnSlice& probe(biome_realm realm, const ChunkPosXZ& pos);
ColumnProbe probe_column(biome_realm realm, const BlockPosXZ& pos);
} // namespace heightmap

constexpr bool Column::is_valid(void) const
{
    return surface_y > UNSET;
}

#endif /* C1DAD708_A904_4335_AD46_212640B0DC2B */
