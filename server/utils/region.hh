#ifndef AEA23A8B_5F80_4F74_A1B6_D0CB7811FB19
#define AEA23A8B_5F80_4F74_A1B6_D0CB7811FB19

#include "shared/coord.hh"

class World;

using RegionPos = Eigen::Vector<std::int32_t, 3>;

struct RegionKey final {
    bool operator==(const RegionKey& other) const;

    const World* world;
    RegionPos pos;
};

template<>
struct std::hash<RegionKey> final {
    std::size_t operator()(const RegionKey& key) const
    {
        std::size_t value = 0;
        value ^= key.pos.x() * 73856093;
        value ^= key.pos.y() * 19349663;
        value ^= key.pos.z() * 83492791;
        value ^= reinterpret_cast<std::size_t>(key.world) * 2246822519;
        return value;
    }
};

namespace utils
{
RegionPos region_of(const ChunkPos& pos);
std::size_t region_slot(const ChunkPos& pos, const RegionPos& region);
} // namespace utils

#endif /* AEA23A8B_5F80_4F74_A1B6_D0CB7811FB19 */
