#ifndef A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5
#define A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5

#include "shared/world/block.hh"

struct LightMaterial final {
    block_light_type opacity;
    block_light_type emission;
};

namespace light_material
{
void init_late(void);
void shutdown(void);
} // namespace light_material

namespace light_material
{
const LightMaterial& get(block_id_type id);
} // namespace light_material

#endif /* A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5 */
