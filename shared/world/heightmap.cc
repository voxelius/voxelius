#include "shared/pch.hh"

#include "shared/world/heightmap.hh"

std::shared_ptr<Heightmap::Entry> Heightmap::find(const ChunkPosXZ& pos) const
{
    std::shared_lock lock(m_mutex);

    auto it = m_entries.find(pos);

    if(it == m_entries.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<Heightmap::Entry> Heightmap::get_or_create(const ChunkPosXZ& pos)
{
    if(auto entry = find(pos)) {
        return entry;
    }

    std::unique_lock lock(m_mutex);

    // Another thread may have inserted this column while this one waited for
    // the exclusive lock. Replacing its entry would free a slice that an
    // in-flight probe still holds a reference to.
    auto it = m_entries.find(pos);

    if(it != m_entries.end()) {
        return it->second;
    }

    auto entry = std::make_shared<Entry>();
    m_entries.emplace(pos, entry);
    return entry;
}

void Heightmap::purge(void)
{
    std::unique_lock lock(m_mutex);

    m_entries.clear();
}
