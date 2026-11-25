#include "shared/pch.hh"

#include "shared/world/light_material.hh"

#include "shared/world/block_collisions.hh"
#include "shared/world/block_registry.hh"

static std::vector<LightMaterial> s_materials;

static block_face opposite_face(block_face face)
{
    switch(face) {
        case BLOCK_FACE_NORTH:
            return BLOCK_FACE_SOUTH;

        case BLOCK_FACE_SOUTH:
            return BLOCK_FACE_NORTH;

        case BLOCK_FACE_EAST:
            return BLOCK_FACE_WEST;

        case BLOCK_FACE_WEST:
            return BLOCK_FACE_EAST;

        case BLOCK_FACE_TOP:
            return BLOCK_FACE_BOTTOM;

        case BLOCK_FACE_BOTTOM:
            return BLOCK_FACE_TOP;
    }

    return BLOCK_FACE_NORTH;
}

// The collision box is the shape the game already knows about, so it decides
// which cell faces the block really closes. A block without one is treated as
// filling its cell, which keeps it as opaque as it has always been
static std::uint8_t make_covered_faces(block_id_type id, const BlockDefinition& def)
{
    if(def.is_stem || def.bcoll_name.is_empty()) {
        return LightMaterial::ALL_FACES;
    }

    auto collision = block_collisions::find(id);

    if(collision == nullptr || collision->bounds.isEmpty()) {
        return LightMaterial::ALL_FACES;
    }

    const auto& bounds = collision->bounds;
    auto full_x = bounds.min().x() <= 0.0f && bounds.max().x() >= 1.0f;
    auto full_y = bounds.min().y() <= 0.0f && bounds.max().y() >= 1.0f;
    auto full_z = bounds.min().z() <= 0.0f && bounds.max().z() >= 1.0f;

    std::uint8_t faces = 0;

    if(full_y && full_z) {
        if(bounds.min().x() <= 0.0f) {
            faces |= 1U << BLOCK_FACE_WEST;
        }

        if(bounds.max().x() >= 1.0f) {
            faces |= 1U << BLOCK_FACE_EAST;
        }
    }

    if(full_x && full_y) {
        if(bounds.min().z() <= 0.0f) {
            faces |= 1U << BLOCK_FACE_NORTH;
        }

        if(bounds.max().z() >= 1.0f) {
            faces |= 1U << BLOCK_FACE_SOUTH;
        }
    }

    if(full_x && full_z) {
        if(bounds.min().y() <= 0.0f) {
            faces |= 1U << BLOCK_FACE_BOTTOM;
        }

        if(bounds.max().y() >= 1.0f) {
            faces |= 1U << BLOCK_FACE_TOP;
        }
    }

    return faces;
}

static LightMaterial make_material(block_id_type id, const BlockDefinition& def)
{
    LightMaterial material {};
    material.opacity = std::min<block_light_type>(def.dissipation, BLOCK_LIGHT_MAX);
    material.emission = std::min<block_light_type>(def.emission, BLOCK_LIGHT_MAX);
    material.covered_faces = make_covered_faces(id, def);

    if(def.render == BLOCK_RENDER_SOLID && material.opacity == BLOCK_LIGHT_MIN) {
        // A solid block without opacity would leak light like air
        material.opacity = BLOCK_LIGHT_MAX;
    }

    return material;
}

void light_material::init_late(void)
{
    auto definitions = block_registry::all_definitions();

    s_materials.clear();
    s_materials.resize(definitions.size());

    for(block_id_type id = 0; id < definitions.size(); id += 1) {
        s_materials[id] = make_material(id, definitions[id]);
    }
}

void light_material::shutdown(void)
{
    s_materials.clear();
}

const LightMaterial& light_material::get(block_id_type id)
{
    constexpr static LightMaterial AIR {
        0,
        0,
        LightMaterial::ALL_FACES,
    };

    if(id >= s_materials.size()) {
        return AIR;
    }

    return s_materials[id];
}

block_light_type light_material::face_cost(block_id_type id, block_face face)
{
    auto& material = get(id);

    if(0 == (material.covered_faces & (1U << face))) {
        return BLOCK_LIGHT_MIN;
    }

    return material.opacity;
}

block_light_type light_material::crossing_cost(block_id_type from, block_id_type to, block_face enter_face)
{
    auto cost = face_cost(to, enter_face);
    auto& from_material = get(from);

    if(from_material.covered_faces == LightMaterial::ALL_FACES) {
        return cost;
    }

    return std::max(cost, face_cost(from, opposite_face(enter_face)));
}

bool light_material::is_opaque(block_id_type id)
{
    auto& material = get(id);

    return material.covered_faces == LightMaterial::ALL_FACES && material.opacity >= BLOCK_LIGHT_MAX;
}
