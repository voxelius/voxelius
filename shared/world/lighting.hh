#ifndef EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718
#define EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718

#include "shared/coord.hh"
#include "shared/world/block.hh"
#include "shared/world/light_storage.hh"

class World;

namespace lighting
{
// The cells of one chunk whose light changed since the last call. A full
// change means the whole array was replaced and a delta is not worth sending.
struct LightChange final {
    bool full { false };
    std::vector<std::uint16_t> indices;
};
} // namespace lighting

namespace lighting
{
void init(void);
void shutdown(void);
} // namespace lighting

namespace lighting
{
void fixed_update(World& world);
} // namespace lighting

namespace lighting
{
// A replica only applies the light it receives and does not run the column
// flood fill itself.
void set_replica(bool value);
} // namespace lighting

namespace lighting
{
// How far light falls when it crosses a face that costs `cost`. Sunlight that
// meets no resistance on its way down keeps its level; every other step costs
// at least one level.
block_light_type step_level(block_light_type level, block_light_type cost, light_channel channel, bool downward);
} // namespace lighting

namespace lighting
{
LightChange take_change(const ChunkPos& pos);
} // namespace lighting

namespace lighting
{
// Drops every queued and in-flight lighting job. Used when the world is
// purged; stale jobs must not touch the next world.
void reset(void);
} // namespace lighting

#endif /* EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718 */
