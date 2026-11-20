#ifndef EA7EF9C5_B33B_4042_8C62_E039B82743C3
#define EA7EF9C5_B33B_4042_8C62_E039B82743C3

#include "core/res/resource.hh"

#include "shared/world/biome.hh"
#include "shared/world/block_storage.hh"

struct Feature;

struct FeatureInstance final {
    BlockPos origin;
    block_face facing;
    res::handle<Feature> feature;
};

class World;

struct PlacementContext final {
    World* world;
    ChunkPos position;
    const BlockStorage* storage;
    std::size_t max_extent;
};

class PlacementRule {
public:
    virtual ~PlacementRule(void) = default;
    virtual void collect(const ChunkPos& pos, const PlacementContext& context, std::vector<FeatureInstance>& out) const = 0;
};

namespace feature_placer::detail
{
void add_rule(std::unique_ptr<PlacementRule> rule);
} // namespace feature_placer::detail

namespace feature_placer
{
Eigen::Vector3i rotate_offset(const Eigen::Vector3i& offset, block_face facing);
Eigen::AlignedBox3i rotate_bounds(const Eigen::AlignedBox3i& bounds, block_face facing);
} // namespace feature_placer

namespace feature_placer
{
void init(void);
void shutdown(void);
} // namespace feature_placer

namespace feature_placer
{
template<typename T, typename... AT>
requires std::derived_from<T, PlacementRule>
void add_rule(AT&&... args);
} // namespace feature_placer

namespace feature_placer
{
void commit(World* world, BlockStorage& storage, const ChunkPos& cpos);
void commit(World* world, BlockStorage& storage, const ChunkPos& cpos, std::span<const FeatureInstance> instances);
} // namespace feature_placer

template<typename T, typename... AT>
requires std::derived_from<T, PlacementRule>
void feature_placer::add_rule(AT&&... args)
{
    feature_placer::detail::add_rule(std::make_unique<T>(std::forward<AT>(args)...));
}

#endif /* EA7EF9C5_B33B_4042_8C62_E039B82743C3 */
