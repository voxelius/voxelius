#ifndef C4E2A19B_5D77_4F0A_9B63_7A1E2C8D4F05
#define C4E2A19B_5D77_4F0A_9B63_7A1E2C8D4F05

#include "shared/coord.hh"
#include "shared/world/block.hh"

namespace fluid_flow
{
template<typename Lookup>
Eigen::Vector3f sample(const BlockDefinition* def, const BlockPos& pos, Lookup&& lookup, fluid_gravity gravity);
} // namespace fluid_flow

template<typename Lookup>
Eigen::Vector3f fluid_flow::sample(const BlockDefinition* def, const BlockPos& pos, Lookup&& lookup, fluid_gravity gravity)
{
    assert(def);

    constexpr auto HEIGHT_SCALE = 0.0625f;
    constexpr auto FALLING_WEIGHT = 6.0f; // Minecraft adds a strong downward vector before normalizing

    auto height = HEIGHT_SCALE * static_cast<float>(def->fluid_level);
    auto vertical = BlockPos(0, 1, 0);
    auto vertical_sign = 1.0f;

    if(gravity == FLUID_GRAVITY_DOWN) {
        vertical = BlockPos(0, -1, 0);
        vertical_sign = -1.0f;
    }

    const std::array<BlockPos, 4> horizontal = {
        BlockPos(1, 0, 0),
        BlockPos(-1, 0, 0),
        BlockPos(0, 0, 1),
        BlockPos(0, 0, -1),
    };

    auto height_of = [](const BlockDefinition* definition) {
        return HEIGHT_SCALE * static_cast<float>(definition->fluid_level);
    };

    Eigen::Vector3f flow = Eigen::Vector3f::Zero();

    for(const auto& dir : horizontal) {
        auto neighbour = lookup(pos + dir);

        if(neighbour && neighbour->collidable) {
            continue;
        }

        if(neighbour && neighbour->fluid == def->fluid && neighbour->fluid_level) {
            auto delta = height - height_of(neighbour);
            flow.x() += delta * static_cast<float>(dir.x());
            flow.z() += delta * static_cast<float>(dir.z());
            continue;
        }

        if(neighbour && neighbour->fluid) {
            continue;
        }

        auto next = lookup(pos + dir + vertical);

        if(next == nullptr || next->fluid != def->fluid || next->fluid_level == 0) {
            continue;
        }

        auto delta = height - height_of(next);
        flow.x() += delta * static_cast<float>(dir.x());
        flow.z() += delta * static_cast<float>(dir.z());
    }

    auto next = lookup(pos + vertical);

    if(def->fluid_falling) {
        flow.y() += vertical_sign * FALLING_WEIGHT;
    }
    else if(next && next->fluid == def->fluid && next->fluid_level) {
        if(!next->collidable) {
            flow.y() += vertical_sign * (height - height_of(next));
        }
    }
    else if(next == nullptr || !next->collidable) {
        flow.y() += vertical_sign * height;
    }

    return flow;
}

#endif /* C4E2A19B_5D77_4F0A_9B63_7A1E2C8D4F05 */
