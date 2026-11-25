#ifndef A159B98F_E5CD_4BCD_9BB1_817385D6A014
#define A159B98F_E5CD_4BCD_9BB1_817385D6A014

#include "shared/coord.hh"

#include "server/constant.hh"

template<unsigned CellW, unsigned CellH>
class NoiseCache2D final {
public:
    static_assert(CellW > 0U);
    static_assert(CellH > 0U);
    static_assert(constant::CHUNK_SIZE % CellW == 0U);
    static_assert(constant::CHUNK_SIZE % CellH == 0U);

    constexpr static unsigned CELL_W = CellW;
    constexpr static unsigned CELL_H = CellH;
    constexpr static unsigned GRID_W = constant::CHUNK_SIZE / CellW + 1U;
    constexpr static unsigned GRID_H = constant::CHUNK_SIZE / CellH + 1U;
    constexpr static std::size_t GRID_SIZE = static_cast<std::size_t>(GRID_W * GRID_H);

    using array_type = std::array<float, GRID_SIZE>;

    static float sample(const array_type& values, const LocalPosXZ& lpos);

    explicit NoiseCache2D(fnl_state noise);

    NoiseCache2D(const NoiseCache2D& other) = delete;
    NoiseCache2D& operator=(const NoiseCache2D& other) = delete;

    NoiseCache2D(NoiseCache2D&& other) = delete;
    NoiseCache2D& operator=(NoiseCache2D&& other) = delete;

    // The reference dies when the entry is erased or purged
    const array_type& get(const ChunkPosXZ& pos);
    const float get_slow(const BlockPosXZ& pos);

    void erase(const ChunkPosXZ& pos);
    void purge(void);

    std::size_t size(void) const;

private:
    struct Entry final {
        std::once_flag init_flag;
        array_type values {};
    };

    std::shared_ptr<Entry> get_or_create(const ChunkPosXZ& pos);

    void generate(const ChunkPosXZ& pos, array_type& values);

    fnl_state m_noise;
    mutable std::mutex m_mutex;
    vx::hash_map<ChunkPosXZ, std::shared_ptr<Entry>> m_cache;
};

using NoiseCache2D_2x2 = NoiseCache2D<2, 2>;
using NoiseCache2D_4x4 = NoiseCache2D<4, 4>;
using NoiseCache2D_8x8 = NoiseCache2D<8, 8>;

#endif /* A159B98F_E5CD_4BCD_9BB1_817385D6A014 */
