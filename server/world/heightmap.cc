#include "server/pch.hh"

#include "server/world/heightmap.hh"

#include "core/utils/modulo.hh"

#include "server/world/realm_surface.hh"

struct HeightmapEntry final {
    std::once_flag init_flag;
    ColumnSlice data;
};

static std::shared_mutex s_mutex;
static vx::hash_map<ChunkPosXZ, std::shared_ptr<HeightmapEntry>> s_cache;

static std::shared_ptr<HeightmapEntry> find_entry(const ChunkPosXZ& pos)
{
    std::shared_lock lock(s_mutex);

    auto it = s_cache.find(pos);

    if(it == s_cache.end()) {
        return nullptr;
    }

    return it->second;
}

static std::shared_ptr<HeightmapEntry> get_entry(const ChunkPosXZ& pos)
{
    if(auto entry = find_entry(pos)) {
        return entry;
    }

    std::unique_lock lock(s_mutex);

    // Another thread may have inserted this column while this one waited for
    // the exclusive lock. Replacing its entry would free a slice that an
    // in-flight probe still holds a reference to.
    auto it = s_cache.find(pos);

    if(it != s_cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<HeightmapEntry>();
    s_cache.emplace(pos, entry);
    return entry;
}

void heightmap::purge(void)
{
    std::unique_lock lock(s_mutex);

    s_cache.clear();
}

const ColumnSlice& heightmap::probe(const ChunkPosXZ& pos)
{
    auto entry = get_entry(pos);

    std::call_once(entry->init_flag, [pos, entry] {
        entry->data = realm_surface::probe(pos);
    });

    return entry->data;
}

const Column& heightmap::probe_slow(const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[1], constant::CHUNK_SIZE));

    auto index = static_cast<std::size_t>(lpos[0] + lpos[1] * constant::CHUNK_SIZE);
    auto& slice = probe(cpos);
    return slice[index];
}
