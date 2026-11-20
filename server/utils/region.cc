#include "server/pch.hh"

#include "server/utils/region.hh"

#include "server/constant.hh"

bool RegionKey::operator==(const RegionKey& other) const
{
    return world == other.world && realm == other.realm && pos == other.pos;
}

RegionPos utils::region_of(const ChunkPos& pos)
{
    RegionPos region;
    region.x() = pos.x() >> constant::REGION_SIZE_LOG2;
    region.y() = pos.y() >> constant::REGION_SIZE_LOG2;
    region.z() = pos.z() >> constant::REGION_SIZE_LOG2;
    return region;
}

std::size_t utils::region_slot(const ChunkPos& pos, const RegionPos& region)
{
    auto lx = static_cast<std::size_t>(pos.x() - (region.x() << constant::REGION_SIZE_LOG2));
    auto ly = static_cast<std::size_t>(pos.y() - (region.y() << constant::REGION_SIZE_LOG2));
    auto lz = static_cast<std::size_t>(pos.z() - (region.z() << constant::REGION_SIZE_LOG2));
    return (ly * constant::REGION_SIZE + lz) * constant::REGION_SIZE + lx;
}
