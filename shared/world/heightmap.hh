#ifndef C1DAD708_A904_4335_AD46_212640B0DC2B
#define C1DAD708_A904_4335_AD46_212640B0DC2B

#include "shared/constant.hh"
#include "shared/coord.hh"

class ReadBuffer;
class WriteBuffer;

constexpr static BlockPos::value_type HEIGHTMAP_UNSET = std::numeric_limits<BlockPos::value_type>::min();

struct Column final {
    constexpr static BlockPos::value_type UNSET = HEIGHTMAP_UNSET;

    static void encode(const Column& column, WriteBuffer& buffer);
    static void decode(Column& column, ReadBuffer& buffer);

    constexpr bool is_valid(void) const;
    constexpr bool shelters(BlockPos::value_type y) const;

    BlockPos::value_type surface_y { UNSET };
    BlockPos::value_type liquid_y { UNSET };
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

constexpr bool Column::shelters(BlockPos::value_type y) const
{
    if(is_valid()) {
        if(liquid_y == UNSET || liquid_y <= surface_y) {
            return y <= surface_y;
        }
        else {
            return y <= liquid_y;
        }
    }

    return false;
}

#endif /* C1DAD708_A904_4335_AD46_212640B0DC2B */
