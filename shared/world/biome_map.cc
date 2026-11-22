#include "shared/pch.hh"

#include "shared/world/biome_map.hh"

#include "shared/utils/coord.hh"

BiomeMap::BiomeMap(void)
{
    m_void = std::make_shared<BiomeSlice>();
}

void BiomeMap::purge(void)
{
    std::scoped_lock lock(m_mutex);

    m_slices.clear();
}

void BiomeMap::attach(entt::registry& registry, entt::entity entity, const ChunkPos& cpos)
{
    BiomeSlice_Component component {};

    auto cpos_xz = ChunkPosXZ(cpos.x(), cpos.z());

    std::scoped_lock lock(m_mutex);

    auto it = m_slices.find(cpos_xz);

    if(it == m_slices.end()) {
        component.ptr = std::make_shared<BiomeSlice>();

        Entry entry {};
        entry.ptr = component.ptr;
        entry.initialized = false;

        m_slices.insert_or_assign(cpos_xz, std::move(entry));
    }
    else {
        component.ptr = it->second.ptr;
    }

    registry.emplace<BiomeSlice_Component>(entity, std::move(component));
}

void BiomeMap::insert(const ChunkPosXZ& pos, BiomeSlice biomes)
{
    std::scoped_lock lock(m_mutex);

    auto it = m_slices.find(pos);

    if(it == m_slices.end()) {
        Entry entry {};
        entry.ptr = std::make_shared<BiomeSlice>(std::move(biomes));
        entry.initialized = true;
        m_slices.insert_or_assign(pos, std::move(entry));
        return;
    }

    // First successful write wins for the column
    if(it->second.initialized) {
        return;
    }

    *it->second.ptr = std::move(biomes);
    it->second.initialized = true;
}

biome_id_type BiomeMap::get_slow(const BlockPos& pos) const
{
    auto bpos_xz = BlockPosXZ(pos.x(), pos.z());
    auto cpos_xz = utils::to_chunk_xz(bpos_xz);
    auto lpos_xz = utils::to_local_xz(bpos_xz);
    auto index_xz = utils::to_index_xz(lpos_xz);

    std::scoped_lock lock(m_mutex);

    auto it = m_slices.find(cpos_xz);

    if(it == m_slices.end()) {
        return BIOME_ID_NULL;
    }

    return (*it->second.ptr)[index_xz];
}

const BiomeSlice& BiomeMap::get(const ChunkPos& pos) const
{
    return get(ChunkPosXZ(pos.x(), pos.z()));
}

const BiomeSlice& BiomeMap::get(const ChunkPosXZ& pos) const
{
    std::scoped_lock lock(m_mutex);

    auto it = m_slices.find(pos);

    if(it == m_slices.end()) {
        return *m_void;
    }

    return *it->second.ptr;
}
