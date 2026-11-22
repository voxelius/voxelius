#include "server/pch.hh"

#include "server/world/noise_cache_2d.hh"

#include "core/utils/modulo.hh"

#include "shared/utils/coord.hh"

template<unsigned CellW, unsigned CellH>
float NoiseCache2D<CellW, CellH>::sample(const array_type& values, const LocalPosXZ& lpos)
{
    auto cell_x = static_cast<unsigned>(lpos[0]) / CELL_W;
    auto cell_y = static_cast<unsigned>(lpos[1]) / CELL_H;

    auto alpha_x = static_cast<float>(lpos[0] % CELL_W) / static_cast<float>(CELL_W);
    auto alpha_y = static_cast<float>(lpos[1] % CELL_H) / static_cast<float>(CELL_H);

    auto v00 = values[GRID_W * (cell_y + 0) + (cell_x + 0)];
    auto v10 = values[GRID_W * (cell_y + 0) + (cell_x + 1)];
    auto v01 = values[GRID_W * (cell_y + 1) + (cell_x + 0)];
    auto v11 = values[GRID_W * (cell_y + 1) + (cell_x + 1)];

    auto v0 = std::lerp(v00, v10, alpha_x);
    auto v1 = std::lerp(v01, v11, alpha_x);
    return std::lerp(v0, v1, alpha_y);
}

template<unsigned CellW, unsigned CellH>
NoiseCache2D<CellW, CellH>::NoiseCache2D(fnl_state noise) : m_noise(std::move(noise))
{
    // empty
}

template<unsigned CellW, unsigned CellH>
const typename NoiseCache2D<CellW, CellH>::array_type& NoiseCache2D<CellW, CellH>::get(const ChunkPosXZ& pos)
{
    auto entry = get_or_create(pos);

    std::call_once(entry->init_flag, [this, pos, entry] {
        generate(pos, entry->values);
    });

    return entry->values;
}

template<unsigned CellW, unsigned CellH>
const float NoiseCache2D<CellW, CellH>::get_slow(const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[1], constant::CHUNK_SIZE));

    auto& values = get(cpos);

    return sample(values, lpos);
}

template<unsigned CellW, unsigned CellH>
void NoiseCache2D<CellW, CellH>::erase(const ChunkPosXZ& pos)
{
    std::scoped_lock lock(m_mutex);

    m_cache.erase(pos);
}

template<unsigned CellW, unsigned CellH>
void NoiseCache2D<CellW, CellH>::purge(void)
{
    std::scoped_lock lock(m_mutex);

    m_cache.clear();
}

template<unsigned CellW, unsigned CellH>
std::size_t NoiseCache2D<CellW, CellH>::size(void) const
{
    std::scoped_lock lock(m_mutex);

    return m_cache.size();
}

template<unsigned CellW, unsigned CellH>
std::shared_ptr<typename NoiseCache2D<CellW, CellH>::Entry> NoiseCache2D<CellW, CellH>::get_or_create(const ChunkPosXZ& pos)
{
    std::scoped_lock lock(m_mutex);

    auto it = m_cache.find(pos);

    if(it == m_cache.cend()) {
        auto entry = std::make_shared<Entry>();
        m_cache.emplace(pos, entry);
        return entry;
    }

    return it->second;
}

template<unsigned CellW, unsigned CellH>
void NoiseCache2D<CellW, CellH>::generate(const ChunkPosXZ& pos, array_type& values)
{
    BlockPosXZ chunk_origin;
    chunk_origin[0] = static_cast<BlockPosXZ::value_type>(pos[0]) * constant::CHUNK_SIZE;
    chunk_origin[1] = static_cast<BlockPosXZ::value_type>(pos[1]) * constant::CHUNK_SIZE;

    fnl_state noise_copy = m_noise; // Copy is intentional - thread safety

    for(unsigned y = 0; y < GRID_H; ++y) {
        for(unsigned x = 0; x < GRID_W; ++x) {
            auto sample_x = static_cast<FNLfloat>(chunk_origin[0] + x * CELL_W);
            auto sample_y = static_cast<FNLfloat>(chunk_origin[1] + y * CELL_H);
            auto index = static_cast<std::size_t>(y * GRID_W + x);
            values[index] = fnlGetNoise2D(&noise_copy, sample_x, sample_y);
        }
    }
}

template class NoiseCache2D<2, 2>;
template class NoiseCache2D<4, 4>;
template class NoiseCache2D<8, 8>;
