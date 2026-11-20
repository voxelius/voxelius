#ifndef C1DAD708_A904_4335_AD46_212640B0DC2B
#define C1DAD708_A904_4335_AD46_212640B0DC2B

#include "shared/constant.hh"
#include "shared/coord.hh"

constexpr static BlockPos::value_type HEIGHTMAP_UNSET = std::numeric_limits<BlockPos::value_type>::min();

struct Column final {
    constexpr static BlockPos::value_type UNSET = HEIGHTMAP_UNSET;

    constexpr bool is_valid(void) const;

    BlockPos::value_type surface_y { UNSET }; // top surface block
    BlockPos::value_type liquid_y { UNSET };  // top liquid block (above surface, otherwise UNSET)
};

using ColumnSlice = std::array<Column, constant::CHUNK_AREA>;

class Heightmap final {
public:
    struct Entry final {
        std::once_flag init_flag;
        ColumnSlice data;
    };

    std::shared_ptr<Entry> find(const ChunkPosXZ& pos) const;
    std::shared_ptr<Entry> get_or_create(const ChunkPosXZ& pos);
    void purge(void);

private:
    mutable std::shared_mutex m_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<Entry>> m_entries;
};

constexpr bool Column::is_valid(void) const
{
    return surface_y > UNSET;
}

#endif /* C1DAD708_A904_4335_AD46_212640B0DC2B */
