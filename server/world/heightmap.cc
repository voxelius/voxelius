#include "server/pch.hh"

#include "server/world/heightmap.hh"

#include "core/utils/modulo.hh"

#include "server/world/realm_sky.hh"
#include "server/world/realm_surface.hh"

struct HeightmapEntry final {
    std::once_flag init_flag;
    ColumnSlice data;
};

static std::shared_mutex s_mutex;
static std::array<vx::hash_map<ChunkPosXZ, std::shared_ptr<HeightmapEntry>>, NUM_BIOME_REALMS> s_cache;

static ColumnSlice generate_slice(biome_realm realm, const ChunkPosXZ& pos)
{
    switch(realm) {
        case BIOME_REALM_SURFACE:
            return realm_surface::probe(pos);

        case BIOME_REALM_SKY:
            return realm_sky::probe(pos);

        case BIOME_REALM_UNDERGROUND:
        case BIOME_REALM_THE_DEPTHS:
        case BIOME_REALM_VOID:
            break; // these realms have no heightmap yet

        case BIOME_REALM_COUNT:
            break; // shut up clang
    }

    return {};
}

static std::shared_ptr<HeightmapEntry> find_entry(biome_realm realm, const ChunkPosXZ& pos)
{
    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    std::shared_lock lock(s_mutex);

    auto& cache = s_cache.at(index);
    auto it = cache.find(pos);

    if(it == cache.end()) {
        return nullptr;
    }

    return it->second;
}

static std::shared_ptr<HeightmapEntry> get_entry(biome_realm realm, const ChunkPosXZ& pos)
{
    if(auto entry = find_entry(realm, pos)) {
        return entry;
    }

    auto index = static_cast<std::size_t>(realm);
    assert(index < NUM_BIOME_REALMS);

    std::unique_lock lock(s_mutex);

    // Another thread may have inserted this column while this one waited for
    // the exclusive lock. Replacing its entry would free a slice that an
    // in-flight probe still holds a reference to.
    auto& cache = s_cache.at(index);
    auto it = cache.find(pos);

    if(it != cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<HeightmapEntry>();
    cache.emplace(pos, entry);
    return entry;
}

void heightmap::purge(void)
{
    std::unique_lock lock(s_mutex);

    for(auto& cache : s_cache) {
        cache.clear();
    }
}

const ColumnSlice& heightmap::probe(biome_realm realm, const ChunkPosXZ& pos)
{
    auto entry = get_entry(realm, pos);

    std::call_once(entry->init_flag, [realm, pos, entry] {
        entry->data = generate_slice(realm, pos);
    });

    return entry->data;
}

const Column& heightmap::probe_slow(biome_realm realm, const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[1], constant::CHUNK_SIZE));

    auto index = static_cast<std::size_t>(lpos[0] + lpos[1] * constant::CHUNK_SIZE);
    auto& slice = probe(realm, cpos);
    return slice[index];
}

ColumnProbe heightmap::probe_column(biome_realm realm, const BlockPosXZ& pos)
{
    switch(realm) {
        case BIOME_REALM_SURFACE:
            return realm_surface::probe_column(pos);

        case BIOME_REALM_SKY:
            return realm_sky::probe_column(pos);

        case BIOME_REALM_UNDERGROUND:
        case BIOME_REALM_THE_DEPTHS:
        case BIOME_REALM_VOID:
            break;

        case BIOME_REALM_COUNT:
            break;
    }

    return {};
}
