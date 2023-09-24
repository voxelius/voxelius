// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <parson.h>
#include <shared/filetools.hh>
#include <shared/vdef.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

static std::unordered_map<std::string, uint16_t> name_map = {};
static vdef_vector voxels = {};

void vdef::purge()
{
    for(const auto it : voxels)
        delete it;
    name_map.clear();
    voxels.clear();
}

uint16_t vdef::assign(const std::string &name, uint16_t hint)
{
    if(hint == NULL_VOXEL_ID) {
        spdlog::error("vdef: cannot assign NULL_VOXEL_ID");
        return NULL_VOXEL_ID;
    }

    if(const auto nit = name_map.find(name); nit != name_map.cend()) {
        spdlog::error("vdef: name {} is taken by voxel {}", nit->first, nit->second);
        return NULL_VOXEL_ID;
    }

    const std::string path1 = fmt::format("/voxels/{}.json", name);
    const std::string path2 = fmt::format("/voxels/{}.jsonc", name);

    std::string source;
    if(!filetools::read(path1, source)) {
        if(!filetools::read(path2, source)) {
            spdlog::error("vdef: {}: {}", name, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return NULL_VOXEL_ID;
        }
    }

    JSON_Value *json_val = json_parse_string_with_comments(source.c_str());
    JSON_Object *json = json_value_get_object(json_val);

    if(!json_val) {
        spdlog::error("vdef: {}: parse error", name);
        return NULL_VOXEL_ID;
    }

    if(!json) {
        json_value_free(json_val);
        spdlog::error("vdef: {}: root is not an object", name);
        return NULL_VOXEL_ID;
    }

    VoxelInfo out = {};
    out.shape = VOXEL_SHAPE_NONE;
    out.states.clear();
    out.name = name;

    const char *shape = json_object_get_string(json, "shape");
    static const std::unordered_map<std::string, voxel_shape_t> shapes = {
        { "none",   VOXEL_SHAPE_NONE    },
        { "cube",   VOXEL_SHAPE_CUBE    },
        { "slab",   VOXEL_SHAPE_SLAB    },
        { "stairs", VOXEL_SHAPE_STAIRS  },
        { "cross",  VOXEL_SHAPE_CROSS   },
        { "liquid", VOXEL_SHAPE_LIQUID  },
    };

    if(const auto it = shapes.find(shape ? shape : std::string{}); it != shapes.cend()) {
        out.shape = it->second;
    }

    const JSON_Array *states = json_object_get_array(json, "states");
    const size_t num_states = json_array_get_count(states);

    for(size_t i = 0; i < num_states; ++i) {
        const JSON_Object *node = json_array_get_object(states, i);

        VoxelInfo::State state = {};
        state.draw = VOXEL_DRAW_NODRAW;
        state.textures.fill(std::string{});
        state.cache.fill(0U);

        const char *draw = json_object_get_string(node, "draw");
        static const std::unordered_map<std::string, voxel_draw_t> draws = {
            { "nodraw", VOXEL_DRAW_NODRAW },
            { "opaque", VOXEL_DRAW_OPAQUE },
            { "alpha", VOXEL_DRAW_ALPHA },
            { "fluid", VOXEL_DRAW_FLUID },
        };

        if(const auto it = draws.find(draw ? draw : std::string{}); it != draws.cend()) {
            state.draw = it->second;
        }

        const JSON_Object *textures = json_object_get_object(node, "textures");
        const size_t num_textures = json_object_get_count(textures);

        for(size_t j = 0; j < num_textures; ++j) {
            const char *face_str = json_object_get_name(textures, j);
            static const std::unordered_map<std::string, voxel_face_t> faces = {
                { "north",      VOXEL_FACE_NORTH    },
                { "south",      VOXEL_FACE_SOUTH    },
                { "east",       VOXEL_FACE_EAST     },
                { "west",       VOXEL_FACE_WEST     },
                { "top",        VOXEL_FACE_TOP      },
                { "bottom",     VOXEL_FACE_BOTTOM   },
                { "cross_1",    VOXEL_FACE_CROSS_1  },
                { "cross_2",    VOXEL_FACE_CROSS_2  },
            };

            const auto it = faces.find(face_str ? face_str : std::string{});
            if(it == faces.cend())
                continue;
            const voxel_face_t face = it->second;

            const std::string texname = json_object_get_name(textures, j);

            const JSON_Value *value = json_object_get_value_at(textures, j);
            const char *cstr = json_value_get_string(value);
            const std::string texture = cstr ? cstr : std::string{};

            if(texture[0] == '^') {
                const std::string query = texture.substr(1);

                if(texname == query) {
                    spdlog::warn("vdef: {}: {}: self-referencing texture", name, texname);
                    continue;
                }

                const char *qstr = json_object_get_string(textures, query.c_str());
                state.textures[face] = qstr ? qstr : texture;
                continue;
            }

            state.textures[face] = texture;
        }

        out.states.push_back(state);
    }

    if(voxels.size() < hint)
        voxels.resize(hint + 1, nullptr);
    voxels[hint] = new VoxelInfo{out};
    name_map[name] = hint;

    return hint;
}

VoxelInfo *vdef::find(const std::string &name)
{
    const auto it = name_map.find(name);
    if(it != name_map.cend())
        return voxels[it->second];
    return nullptr;
}

VoxelInfo *vdef::find(uint16_t id)
{
    if(id < voxels.size())
        return voxels[id];
    return nullptr;
}

vdef_iterator vdef::begin()
{
    return voxels.begin();
}

vdef_iterator vdef::end()
{
    return voxels.end();
}
