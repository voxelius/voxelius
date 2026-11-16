#include "shared/pch.hh"

#include "shared/world/light_material.hh"

#include "shared/world/block_registry.hh"

static std::vector<LightMaterial> s_materials;

static LightMaterial make_material(const BlockDefinition& def)
{
    LightMaterial material {};
    material.opacity = std::min<block_light_type>(def.dissipation, BLOCK_LIGHT_MAX);

    if(def.render == BLOCK_RENDER_SOLID && material.opacity == BLOCK_LIGHT_MIN) {
        // A solid block without opacity would leak light like air.
        material.opacity = BLOCK_LIGHT_MAX;
    }

    material.emission = std::min<block_light_type>(def.emission, BLOCK_LIGHT_MAX);

    return material;
}

void light_material::init_late(void)
{
    auto definitions = block_registry::all_definitions();

    s_materials.clear();
    s_materials.resize(definitions.size());

    for(block_id_type id = 0; id < definitions.size(); id += 1) {
        s_materials[id] = make_material(definitions[id]);
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
    };

    if(id >= s_materials.size()) {
        return AIR;
    }

    return s_materials[id];
}
