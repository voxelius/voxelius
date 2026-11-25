#include "shared/pch.hh"

#include "shared/world/heightmap.hh"

#include "core/buffer.hh"

constexpr static std::int32_t HEIGHT_NULL = INT32_MIN;

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
    // in-flight probe still holds a reference to
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

void Column::encode(const Column& column, WriteBuffer& buffer)
{
    auto surface_y = HEIGHT_NULL;
    auto liquid_y = HEIGHT_NULL;

    if(column.surface_y != UNSET) {
        surface_y = static_cast<std::int32_t>(column.surface_y);
    }

    if(column.liquid_y != UNSET) {
        liquid_y = static_cast<std::int32_t>(column.liquid_y);
    }

    buffer.write<std::int32_t>(surface_y);
    buffer.write<std::int32_t>(liquid_y);
}

void Column::decode(Column& column, ReadBuffer& buffer)
{
    auto surface_y = buffer.read<std::int32_t>();
    auto liquid_y = buffer.read<std::int32_t>();

    column.surface_y = UNSET;
    column.liquid_y = UNSET;

    if(surface_y != HEIGHT_NULL) {
        column.surface_y = static_cast<BlockPos::value_type>(surface_y);
    }

    if(liquid_y != HEIGHT_NULL) {
        column.liquid_y = static_cast<BlockPos::value_type>(liquid_y);
    }
}