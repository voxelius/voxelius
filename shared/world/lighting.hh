#ifndef EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718
#define EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718

#include "shared/coord.hh"
#include "shared/world/block.hh"

namespace lighting
{
void init(void);
void shutdown(void);
} // namespace lighting

namespace lighting
{
void fixed_update(void);
} // namespace lighting

namespace lighting
{
// A replica only applies the light it receives and does not run the column
// flood fill itself.
void set_replica(bool value);
} // namespace lighting

namespace lighting
{
// How much light the block absorbs when it is traversed.
block_light_type attenuation(block_id_type id);
} // namespace lighting

namespace lighting
{
// Drops every queued and in-flight lighting job. Used when the world is
// purged; stale jobs must not touch the next world.
void reset(void);
} // namespace lighting

#endif /* EE3F4A5C_6D7E_4F80_91B2_C3D4E5F60718 */
