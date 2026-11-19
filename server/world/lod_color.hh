#ifndef F1A3C5E7_2B49_4D68_9C01_E2F3A4B5C6D7
#define F1A3C5E7_2B49_4D68_9C01_E2F3A4B5C6D7

#include "shared/world/biome.hh"
#include "shared/world/block.hh"

namespace lod_color
{
void init(void);
void shutdown(void);
} // namespace lod_color

namespace lod_color
{
Eigen::Vector3f block_color(block_id_type id, biome_id_type biome = BIOME_ID_NULL);
} // namespace lod_color

#endif /* F1A3C5E7_2B49_4D68_9C01_E2F3A4B5C6D7 */
