#include "server/pch.hh"

#include "server/world/noise_cache_3d.hh"

#include "core/utils/modulo.hh"

#include "shared/utils/coord.hh"

template<unsigned CellW, unsigned CellH, unsigned CellD>
float NoiseCache3D<CellW, CellH, CellD>::sample(const array_type& values, const LocalPos& lpos)
{
    auto cell_x = static_cast<unsigned>(lpos.x()) / CELL_W;
    auto cell_y = static_cast<unsigned>(lpos.y()) / CELL_H;
    auto cell_z = static_cast<unsigned>(lpos.z()) / CELL_D;

    auto alpha_x = static_cast<float>(lpos.x() % CELL_W) / static_cast<float>(CELL_W);
    auto alpha_y = static_cast<float>(lpos.y() % CELL_H) / static_cast<float>(CELL_H);
    auto alpha_z = static_cast<float>(lpos.z() % CELL_D) / static_cast<float>(CELL_D);

    auto v000 = values[GRID_W * (GRID_H * (cell_z + 0) + (cell_y + 0)) + (cell_x + 0)];
    auto v100 = values[GRID_W * (GRID_H * (cell_z + 0) + (cell_y + 0)) + (cell_x + 1)];
    auto v010 = values[GRID_W * (GRID_H * (cell_z + 0) + (cell_y + 1)) + (cell_x + 0)];
    auto v110 = values[GRID_W * (GRID_H * (cell_z + 0) + (cell_y + 1)) + (cell_x + 1)];
    auto v001 = values[GRID_W * (GRID_H * (cell_z + 1) + (cell_y + 0)) + (cell_x + 0)];
    auto v101 = values[GRID_W * (GRID_H * (cell_z + 1) + (cell_y + 0)) + (cell_x + 1)];
    auto v011 = values[GRID_W * (GRID_H * (cell_z + 1) + (cell_y + 1)) + (cell_x + 0)];
    auto v111 = values[GRID_W * (GRID_H * (cell_z + 1) + (cell_y + 1)) + (cell_x + 1)];

    auto v00 = std::lerp(v000, v100, alpha_x);
    auto v10 = std::lerp(v010, v110, alpha_x);
    auto v01 = std::lerp(v001, v101, alpha_x);
    auto v11 = std::lerp(v011, v111, alpha_x);

    auto v0 = std::lerp(v00, v10, alpha_y);
    auto v1 = std::lerp(v01, v11, alpha_y);

    return std::lerp(v0, v1, alpha_z);
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
NoiseCache3D<CellW, CellH, CellD>::NoiseCache3D(fnl_state noise) : m_noise(std::move(noise))
{
    // empty
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
const typename NoiseCache3D<CellW, CellH, CellD>::array_type& NoiseCache3D<CellW, CellH, CellD>::get(const ChunkPos& pos)
{
    auto entry = get_or_create(pos);

    std::call_once(entry->init_flag, [this, pos, entry] {
        generate(pos, entry->values);
    });

    return entry->values;
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
const float NoiseCache3D<CellW, CellH, CellD>::get_slow(const BlockPos& pos)
{
    auto cpos = utils::to_chunk(pos);
    auto lpos = utils::to_local(pos);

    auto& values = get(cpos);

    return sample(values, lpos);
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
void NoiseCache3D<CellW, CellH, CellD>::erase(const ChunkPos& pos)
{
    std::scoped_lock lock(m_mutex);

    m_cache.erase(pos);
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
void NoiseCache3D<CellW, CellH, CellD>::purge(void)
{
    std::scoped_lock lock(m_mutex);

    m_cache.clear();
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
std::size_t NoiseCache3D<CellW, CellH, CellD>::size(void) const
{
    std::scoped_lock lock(m_mutex);

    return m_cache.size();
}

template<unsigned CellW, unsigned CellH, unsigned CellD>
std::shared_ptr<typename NoiseCache3D<CellW, CellH, CellD>::Entry> NoiseCache3D<CellW, CellH, CellD>::get_or_create(const ChunkPos& pos)
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

template<unsigned CellW, unsigned CellH, unsigned CellD>
void NoiseCache3D<CellW, CellH, CellD>::generate(const ChunkPos& pos, array_type& values)
{
    auto bpos = utils::to_block(pos);

    fnl_state noise = m_noise; // Copy is intentional - thread safety

    for(unsigned z = 0U; z < GRID_D; ++z) {
        for(unsigned y = 0U; y < GRID_H; ++y) {
            for(unsigned x = 0U; x < GRID_W; ++x) {
                auto sample_x = static_cast<FNLfloat>(bpos.x() + x * CELL_W);
                auto sample_y = static_cast<FNLfloat>(bpos.y() + y * CELL_H);
                auto sample_z = static_cast<FNLfloat>(bpos.z() + z * CELL_D);
                auto index = static_cast<std::size_t>(GRID_W * (GRID_H * z + y) + x);
                values[index] = fnlGetNoise3D(&noise, sample_x, sample_y, sample_z);
            }
        }
    }
}

template class NoiseCache3D<2, 2, 2>;
template class NoiseCache3D<4, 4, 4>;
template class NoiseCache3D<8, 8, 8>;

template class NoiseCache3D<2, 4, 2>;
template class NoiseCache3D<4, 8, 4>;
