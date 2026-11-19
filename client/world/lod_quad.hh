#ifndef B4D6F8A2_3C51_4E79_8A10_D2E4F6A8B935
#define B4D6F8A2_3C51_4E79_8A10_D2E4F6A8B935

constexpr static std::uint32_t LOD_FACE_TOP = 0;
constexpr static std::uint32_t LOD_FACE_BOTTOM = 1;
constexpr static std::uint32_t LOD_FACE_NORTH = 2;
constexpr static std::uint32_t LOD_FACE_SOUTH = 3;
constexpr static std::uint32_t LOD_FACE_EAST = 4;
constexpr static std::uint32_t LOD_FACE_WEST = 5;

constexpr static std::uint32_t LOD_QUAD_WATER_BIT = 1U << 24U;
constexpr static std::uint32_t LOD_QUAD_COLOR_MASK = 0x00FFFFFFU;

struct LodChunkQuad final {
    std::uint32_t data_cell;
    std::uint32_t data_color;
    std::int32_t origin_x;
    std::int32_t origin_y;
    std::int32_t origin_z;
};

struct LodTileQuad final {
    std::uint32_t data_cell;
    std::uint32_t data_height;
    std::uint32_t data_color;
    std::int32_t origin_x;
    std::int32_t origin_z;
};

static_assert(sizeof(LodChunkQuad) == 20, "LodChunkQuad must stay packed for the instanced layout");
static_assert(sizeof(LodTileQuad) == 20, "LodTileQuad must stay packed for the instanced layout");

#endif /* B4D6F8A2_3C51_4E79_8A10_D2E4F6A8B935 */
