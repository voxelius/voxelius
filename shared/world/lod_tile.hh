#ifndef E6C2A8F1_3B47_4D59_8E20_A9C1B3D5F742
#define E6C2A8F1_3B47_4D59_8E20_A9C1B3D5F742

#include "shared/world/biome.hh"
#include "shared/world/lod.hh"

class ReadBuffer;
class WriteBuffer;

constexpr static std::size_t LOD_TILE_CELLS = 32;
constexpr static std::size_t LOD_TILE_CELL_COUNT = LOD_TILE_CELLS * LOD_TILE_CELLS;
constexpr static std::uint8_t LOD_TILE_LEVEL_MAX = 8;

// Bumped when the tile payload or the way its cells are sampled changes. The
// files under save/lod/tile are kept between runs, so a stale version would
// leave the old geometry in place where the player has already been.
constexpr static std::uint8_t LOD_TILE_VERSION = 3;
constexpr static std::int16_t LOD_CELL_EMPTY = std::numeric_limits<std::int16_t>::min();
constexpr static std::uint8_t LOD_CELL_WATER = 1U << 0U;

struct LodKey final {
    bool operator==(const LodKey& other) const;

    biome_realm realm;
    std::uint8_t level;
    std::int32_t x;
    std::int32_t z;
};

struct LodTileCell final {
    std::int16_t height;
    std::int16_t bottom;
    std::uint8_t color;
    std::uint8_t flags;

    constexpr bool is_empty(void) const;
    constexpr bool is_water(void) const;
};

struct LodTile final {
    static bool encode(const LodTile& tile, WriteBuffer& buffer);
    static bool decode(LodTile& tile, ReadBuffer& buffer);

    LodKey key {};
    std::uint8_t color_mode { lod::COLOR_PALETTE8 };
    LodPalette palette;
    std::array<LodTileCell, LOD_TILE_CELL_COUNT> cells;
};

template<>
struct std::hash<LodKey> final {
    std::size_t operator()(const LodKey& key) const
    {
        std::size_t value = 0;
        value ^= static_cast<std::size_t>(key.realm) * 2654435761;
        value ^= static_cast<std::size_t>(key.level) * 2246822519;
        value ^= static_cast<std::size_t>(key.x) * 73856093;
        value ^= static_cast<std::size_t>(key.z) * 83492791;
        return value;
    }
};

namespace lod
{
constexpr std::int32_t tile_cell_size(std::uint8_t level);
constexpr std::int32_t tile_edge_chunks(std::uint8_t level);
constexpr std::int32_t tile_edge_blocks(std::uint8_t level);
} // namespace lod

constexpr std::int32_t lod::tile_cell_size(std::uint8_t level)
{
    return static_cast<std::int32_t>(1) << level;
}

constexpr std::int32_t lod::tile_edge_blocks(std::uint8_t level)
{
    return static_cast<std::int32_t>(LOD_TILE_CELLS) * tile_cell_size(level);
}

constexpr std::int32_t lod::tile_edge_chunks(std::uint8_t level)
{
    return tile_edge_blocks(level) >> 4;
}

constexpr bool LodTileCell::is_empty(void) const
{
    return height == LOD_CELL_EMPTY;
}

constexpr bool LodTileCell::is_water(void) const
{
    return flags & LOD_CELL_WATER;
}

#endif /* E6C2A8F1_3B47_4D59_8E20_A9C1B3D5F742 */
