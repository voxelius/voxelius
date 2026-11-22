#ifndef F63690CB_F2D8_41C3_88F8_CCA3EBDBDC80
#define F63690CB_F2D8_41C3_88F8_CCA3EBDBDC80

#include "server/world/placement_rule.hh"

class PlacementScatter final : public PlacementRule {
public:
    virtual ~PlacementScatter(void) override = default;
    virtual void collect(const ChunkPos& pos, const PlacementContext& context, std::vector<FeatureInstance>& out) const override;
};

#endif /* F63690CB_F2D8_41C3_88F8_CCA3EBDBDC80 */
