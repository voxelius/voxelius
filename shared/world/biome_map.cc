#include "shared/pch.hh"

#include "shared/world/biome_map.hh"

#include "shared/utils/biome.hh"
#include "shared/utils/coord.hh"

BiomeMap::BiomeMap(void)
{
    m_void = std::make_shared<BiomeSlice>();
}

void BiomeMap::purge(void)
{
    std::scoped_lock lock(m_mutex);

    for(auto& slices : m_slices) {
        slices.clear();
    }
}

void BiomeMap::attach(entt::registry& registry, entt::entity entity, const ChunkPos& cpos)
{
    auto realm = utils::realm(cpos.y());

    BiomeSlice_Component component {};

    if(realm == BIOME_REALM_VOID) {
        component.ptr = m_void;
        registry.emplace<BiomeSlice_Component>(entity, std::move(component));
        return;
    }

    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    auto cpos_xz = ChunkPosXZ(cpos.x(), cpos.z());

    std::scoped_lock lock(m_mutex);

    auto& slices = m_slices[index];
    auto it = slices.find(cpos_xz);

    if(it == slices.end()) {
        component.ptr = std::make_shared<BiomeSlice>();

        Entry entry {};
        entry.ptr = component.ptr;
        entry.initialized = false;

        slices.insert_or_assign(cpos_xz, std::move(entry));
    }
    else {
        component.ptr = it->second.ptr;
    }

    registry.emplace<BiomeSlice_Component>(entity, std::move(component));
}

void BiomeMap::insert(biome_realm realm, const ChunkPosXZ& pos, BiomeSlice biomes)
{
    if(realm == BIOME_REALM_VOID) {
        return;
    }

    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    std::scoped_lock lock(m_mutex);

    auto& slices = m_slices[index];
    auto it = slices.find(pos);

    if(it == slices.end()) {
        Entry entry {};
        entry.ptr = std::make_shared<BiomeSlice>(std::move(biomes));
        entry.initialized = true;
        slices.insert_or_assign(pos, std::move(entry));
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
    auto cpos = utils::to_chunk(pos);
    auto realm = utils::realm(cpos.y());
    return get_slow(realm, BlockPosXZ(pos.x(), pos.z()));
}

biome_id_type BiomeMap::get_slow(biome_realm realm, const BlockPosXZ& pos) const
{
    if(realm == BIOME_REALM_VOID) {
        return BIOME_ID_NULL;
    }

    auto cpos_xz = utils::to_chunk_xz(pos);
    auto lpos_xz = utils::to_local_xz(pos);
    auto index_xz = utils::to_index_xz(lpos_xz);

    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    std::scoped_lock lock(m_mutex);

    auto& slices = m_slices[index];
    auto it = slices.find(cpos_xz);

    if(it == slices.end()) {
        return BIOME_ID_NULL;
    }

    return (*it->second.ptr)[index_xz];
}

const BiomeSlice& BiomeMap::get(const ChunkPos& pos) const
{
    auto realm = utils::realm(pos.y());
    return get(realm, ChunkPosXZ(pos.x(), pos.z()));
}

const BiomeSlice& BiomeMap::get(biome_realm realm, const ChunkPosXZ& pos) const
{
    if(realm == BIOME_REALM_VOID) {
        return *m_void;
    }

    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    std::scoped_lock lock(m_mutex);

    auto& slices = m_slices[index];
    auto it = slices.find(pos);

    if(it == slices.end()) {
        return *m_void;
    }

    return *it->second.ptr;
}
