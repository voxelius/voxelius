#ifndef DEF3C4C8_98BE_4D8B_8FB6_C5AD91C14873
#define DEF3C4C8_98BE_4D8B_8FB6_C5AD91C14873

#include "shared/coord.hh"

#include "server/constant.hh"

template<unsigned CellW, unsigned CellH, unsigned CellD>
class NoiseCache3D final {
public:
    static_assert(CellW > 0U);
    static_assert(CellH > 0U);
    static_assert(CellD > 0U);
    static_assert(constant::CHUNK_SIZE % CellW == 0U);
    static_assert(constant::CHUNK_SIZE % CellH == 0U);
    static_assert(constant::CHUNK_SIZE % CellD == 0U);

    constexpr static unsigned CELL_W = CellW;
    constexpr static unsigned CELL_H = CellH;
    constexpr static unsigned CELL_D = CellD;
    constexpr static unsigned GRID_W = constant::CHUNK_SIZE / CellW + 1U;
    constexpr static unsigned GRID_H = constant::CHUNK_SIZE / CellH + 1U;
    constexpr static unsigned GRID_D = constant::CHUNK_SIZE / CellD + 1U;
    constexpr static std::size_t GRID_SIZE = static_cast<std::size_t>(GRID_W * GRID_H * GRID_D);

    using array_type = std::array<float, GRID_SIZE>;

    static float sample(const array_type& values, const LocalPos& lpos);

    explicit NoiseCache3D(fnl_state noise);

    NoiseCache3D(const NoiseCache3D& other) = delete;
    NoiseCache3D& operator=(const NoiseCache3D& other) = delete;

    NoiseCache3D(NoiseCache3D&& other) = delete;
    NoiseCache3D& operator=(NoiseCache3D&& other) = delete;

    const array_type& get(const ChunkPos& pos);
    const float get_slow(const BlockPos& pos);

    void erase(const ChunkPos& pos);
    void purge(void);

    std::size_t size(void) const;

private:
    struct Entry final {
        std::once_flag init_flag;
        array_type values {};
    };

    std::shared_ptr<Entry> get_or_create(const ChunkPos& pos);

    void generate(const ChunkPos& pos, array_type& values);

    fnl_state m_noise;
    mutable std::mutex m_mutex;
    vx::hash_map<ChunkPos, std::shared_ptr<Entry>> m_cache;
};

using NoiseCache3D_2x2x2 = NoiseCache3D<2, 2, 2>;
using NoiseCache3D_4x4x4 = NoiseCache3D<4, 4, 4>;
using NoiseCache3D_8x8x8 = NoiseCache3D<8, 8, 8>;

using NoiseCache3D_2x4x2 = NoiseCache3D<2, 4, 2>;
using NoiseCache3D_4x8x4 = NoiseCache3D<4, 8, 4>;

#endif /* DEF3C4C8_98BE_4D8B_8FB6_C5AD91C14873 */
