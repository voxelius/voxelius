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

static std::unordered_map<Identifier, uint16_t> name_map = {};
static vdef_vector voxels = {};

void vdef::purge()
{
    for(const auto it : voxels)
        delete it;
    name_map.clear();
    voxels.clear();
}

uint16_t vdef::assign(const Identifier &name, uint16_t hint)
{
    if(hint == NULL_VOXEL_ID) {
        spdlog::error("vdef: cannot assign NULL_VOXEL_ID");
        return NULL_VOXEL_ID;
    }

    if(const auto nit = name_map.find(name); nit != name_map.cend()) {
        spdlog::error("vdef: name {} is taken by voxel {}", nit->first.get(), nit->second);
        return NULL_VOXEL_ID;
    }

    const std::string path1 = name.get_file_path_ext("voxels", "json");
    const std::string path2 = name.get_file_path_ext("voxels", "jsonc");

    std::string source;
    if(!util::read(path1, source)) {
        if(!util::read(path2, source)) {
            spdlog::error("vdef: {}: {}", name.get(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return NULL_VOXEL_ID;
        }
    }

    JSON_Value *json_val = json_parse_string_with_comments(source.c_str());
    JSON_Object *json = json_value_get_object(json_val);

    if(!json_val) {
        spdlog::error("vdef: {}: parse error", name.get());
        return NULL_VOXEL_ID;
    }

    if(!json) {
        json_value_free(json_val);
        spdlog::error("vdef: {}: root is not an object", name.get());
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
        state.cache.fill(UINT32_MAX);

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
            const auto face = it->second;

            const std::string texname = json_object_get_name(textures, j);

            const JSON_Value *value = json_object_get_value_at(textures, j);
            const char *cstr = json_value_get_string(value);
            const std::string texture = cstr ? cstr : std::string{};

            if(texture[0] == '^') {
                const std::string query = texture.substr(1);

                if(texname == query) {
                    spdlog::warn("vdef: {}: {}: self-referencing texture", name.get(), texname);
                    continue;
                }

                const char *qstr = json_object_get_string(textures, query.c_str());
                const Identifier ident = Identifier{qstr ? qstr : texture};
                state.textures[face] = ident.get_file_path("textures");
                spdlog::debug("vdef: {}: {} -> {}", name.get(), texname, state.textures[face]);
            }
            else {
                const Identifier ident = Identifier{texture};
                state.textures[face] = ident.get_file_path("textures");
                spdlog::debug("vdef: {}: {} -> {}", name.get(), texname, state.textures[face]);
            }
        }

        out.states.push_back(state);
    }

    if(voxels.size() < hint)
        voxels.resize(hint + 1, nullptr);
    voxels[hint] = new VoxelInfo{out};
    name_map[name] = hint;

    return hint;
}

VoxelInfo *vdef::find(const Identifier &name)
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
