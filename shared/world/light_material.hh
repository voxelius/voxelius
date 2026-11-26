#ifndef A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5
#define A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5

#include "shared/world/block.hh"

struct LightMaterial final {
    static constexpr std::uint8_t ALL_FACES = 0x3F;

    block_light_type opacity;
    block_light_type emission;

    // Faces of the cell the solid part of the block covers. A face the block
    // leaves open costs no light, however opaque the covered part is: a slab
    // fills the bottom half of its cell, so only its bottom face is closed
    std::uint8_t covered_faces;
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

namespace light_material
{
block_light_type face_cost(block_id_type id, block_face face);

// Light absorbed by a step from `from` into `to` through `enter_face`, the
// face of `to` that the step crosses. A block that fills its cell does not
// block the light it sits in, or an emitter would not shine; a partial block
// blocks only across the face its solid part covers
block_light_type crossing_cost(block_id_type from, block_id_type to, block_face enter_face);

bool is_opaque(block_id_type id);
} // namespace light_material

#endif /* A1B2C3D4_E5F6_4A7B_8C9D_E0F1A2B3C4D5 */
