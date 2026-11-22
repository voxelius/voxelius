#ifndef EA7EF9C5_B33B_4042_8C62_E039B82743C3
#define EA7EF9C5_B33B_4042_8C62_E039B82743C3

#include "core/res/resource.hh"

#include "shared/coord.hh"
#include "shared/world/biome.hh"

class BlockStorage;
class ServerWorld;
struct Feature;

struct FeatureInstance final {
    BlockPos origin;
    block_face facing;
    res::handle<Feature> feature;
};

struct PlacementContext final {
    ServerWorld* world;
    ChunkPos position;
    const BlockStorage* storage;
    std::size_t max_extent;
};

class PlacementRule {
public:
    virtual ~PlacementRule(void) = default;
    virtual void collect(const ChunkPos& pos, const PlacementContext& context, std::vector<FeatureInstance>& out) const = 0;
};

#endif /* EA7EF9C5_B33B_4042_8C62_E039B82743C3 */
