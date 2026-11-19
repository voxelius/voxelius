#ifndef D3E5B7A9_1C48_4F62_8A70_5B9D2E4C6F31
#define D3E5B7A9_1C48_4F62_8A70_5B9D2E4C6F31

#include "shared/world/lod.hh"

class ReadBuffer;
class WriteBuffer;

constexpr static std::uint8_t CHUNK_LOD_VERSION = 2;
constexpr static std::uint8_t CHUNK_LOD_LEVEL_MIN = 0;
constexpr static std::uint8_t CHUNK_LOD_LEVEL_MAX = 4;
constexpr static std::size_t CHUNK_LOD_TOTAL_CELLS = 4681;
constexpr static std::uint8_t CHUNK_LOD_COVERAGE_MASK = 0x0F;
constexpr static std::uint8_t CHUNK_LOD_WATER_BIT = 1U << 4U;
constexpr static std::uint8_t CHUNK_LOD_SOLID_THRESHOLD = 8;

std::size_t chunk_lod_level_volume(std::uint8_t level);

struct ChunkLodCell final {
    std::uint8_t color;
    std::uint8_t coverage_flags;

    constexpr std::uint32_t coverage(void) const;
    constexpr bool is_water(void) const;
    constexpr bool is_solid(void) const;
};

struct ChunkLod final {
    static bool encode(const ChunkLod& lod, WriteBuffer& buffer);
    static bool decode(ChunkLod& lod, ReadBuffer& buffer);

    std::span<ChunkLodCell> cells(std::uint8_t level);
    std::span<const ChunkLodCell> cells(std::uint8_t level) const;

    std::uint8_t color_mode { lod::COLOR_PALETTE8 };
    LodPalette palette;

    std::array<ChunkLodCell, 4096> level0;
    std::array<ChunkLodCell, 512> level1;
    std::array<ChunkLodCell, 64> level2;
    std::array<ChunkLodCell, 8> level3;
    std::array<ChunkLodCell, 1> level4;
};

constexpr std::uint32_t ChunkLodCell::coverage(void) const
{
    return coverage_flags & CHUNK_LOD_COVERAGE_MASK;
}

constexpr bool ChunkLodCell::is_water(void) const
{
    return coverage_flags & CHUNK_LOD_WATER_BIT;
}

constexpr bool ChunkLodCell::is_solid(void) const
{
    return coverage() >= CHUNK_LOD_SOLID_THRESHOLD;
}

#endif /* D3E5B7A9_1C48_4F62_8A70_5B9D2E4C6F31 */
