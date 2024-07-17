// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <parson.h>
#include <physfs.h>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

std::unordered_map<std::string, VoxelBuilder> vdef::builders = {};
std::unordered_set<vfs::path_t, vfs::path_hasher_t> vdef::textures = {};
std::vector<VoxelInfo> vdef::voxels = {};

static bool parse_draw(const JSON_Object *state, VoxelDraw &out)
{
    static const std::unordered_map<std::string, VoxelDraw> draws = {
        { "nodraw", VOXEL_DRAW_NODRAW   },
        { "opaque", VOXEL_DRAW_OPAQUE   },
        { "thres",  VOXEL_DRAW_THRES    },
        { "blend",  VOXEL_DRAW_BLEND    },
    };

    if(const char *draw = json_object_get_string(state, "draw")) {
        const auto it = draws.find(draw);

        if(it != draws.cend()) {
            out = it->second;
            return true;
        }
    }

    return false;
}

static bool parse_texture(const JSON_Array *texture, std::vector<vfs::path_t> &out)
{
    if(std::size_t count = json_array_get_count(texture)) {
        out.clear();

        for(std::size_t i = 0; i < count; ++i) {
            if(const char *str = json_array_get_string(texture, i)) {
                vdef::textures.insert(str);
                out.push_back(str);
                continue;
            }

            return false;
        }

        return true;
    }

    return false;
}

VoxelBuilder::VoxelBuilder(VoxelType type, const std::string &name)
{
    this->type = type;
    this->name = name;
}

VoxelBuilder &VoxelBuilder::state_default(void)
{
    states.insert("default");
    return (*this);
}

VoxelBuilder &VoxelBuilder::state(const std::string &name)
{
    states.insert(name);
    return (*this);
}

Voxel VoxelBuilder::build(void) const
{
    const std::string path = fmt::format("/voxels/{}.json", name);

    PHYSFS_File *file = PHYSFS_openRead(path.c_str());

    if(!file) {
        spdlog::warn("vdef: {}: {}", path, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return NULL_VOXEL;
    }

    std::string source = {};
    source.resize(PHYSFS_fileLength(file));
    PHYSFS_readBytes(file, source.data(), source.size());
    PHYSFS_close(file);

    JSON_Value *jsonv = json_parse_string(source.c_str());
    JSON_Object *json = json_value_get_object(jsonv);

    if(!jsonv) {
        spdlog::warn("vdef: {}: parse error", path);
        return NULL_VOXEL;
    }

    if(!json) {
        spdlog::warn("vdef: {}: root is not an object", path);
        json_value_free(jsonv);
        return NULL_VOXEL;
    }

    if((vdef::voxels.size() + states.size()) >= VOXEL_MAX) {
        spdlog::critical("vdef: voxel registry overflow");
        json_value_free(jsonv);
        std::terminate();
    }

    std::vector<VoxelInfo> infos = {};
    Voxel base_voxel = vdef::voxels.size() + 1;

    for(const std::string &state_name : states) {
        VoxelInfo info = {};
        info.name = name;
        info.type = type;
        info.base_voxel = base_voxel;

        const JSON_Object *state = json_object_get_object(json, state_name.c_str());

        if(!state) {
            spdlog::error("vdef: {}: {} is not an object", path, state_name);
            json_value_free(jsonv);
            return NULL_VOXEL;
        }

        if(!parse_draw(state, info.draw)) {
            spdlog::warn("vdef: {}[{}]: invalid or undefined draw mode", path, state_name);
            info.draw = VOXEL_DRAW_NODRAW;
        }

        const JSON_Object *textures = json_object_get_object(state, "textures");
        const std::size_t num_textures = json_object_get_count(textures);

        if(const JSON_Array *texture = json_object_get_array(textures, "default")) {
            std::vector<vfs::path_t> vec = {};

            if(parse_texture(texture, vec)) {
                for(VoxelTexture &vtex : info.textures) {
                    vtex.paths.assign(vec.cbegin(), vec.cend());
                    vtex.cached_offset = 0;
                }
            }
        }

        for(std::size_t i = 0; i < num_textures; ++i) {
            const char *cstr = json_object_get_name(textures, i);
            const std::string str = cstr ? cstr : std::string("[NULL]");

            if(!str.compare("default")) {
                // If we came to this point, the default
                // texture is already parsed
                continue;
            }

            static const std::unordered_map<std::string, VoxelFace> faces = {
                { "north",      VOXEL_FACE_NORTH    },
                { "south",      VOXEL_FACE_SOUTH    },
                { "east",       VOXEL_FACE_EAST     },
                { "west",       VOXEL_FACE_WEST     },
                { "top",        VOXEL_FACE_TOP      },
                { "bottom",     VOXEL_FACE_BOTTOM   },
                { "custom_x",   VOXEL_FACE_CUSTOM_X },
                { "custom_y",   VOXEL_FACE_CUSTOM_Y },
            };

            const auto it = faces.find(str);

            if(it == faces.cend()) {
                spdlog::warn("vdef: {}[{}]: invalid texture name {}", path, state_name, str);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            const JSON_Value *texturev = json_object_get_value_at(textures, i);
            const JSON_Array *texture = json_value_get_array(texturev);

            if(!texturev || !texture) {
                spdlog::warn("vdef: {}[{}]: invalid texture {}", path, state_name, str);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            if(!parse_texture(texture, info.textures[it->second].paths)) {
                spdlog::warn("vdef: {}[{}]: texture {}: parse error", path, state_name, str);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            info.textures[it->second].cached_offset = 0;
        }

        infos.push_back(info);
    }

    json_value_free(jsonv);

    vdef::voxels.insert(vdef::voxels.end(), infos.cbegin(), infos.cend());

    return base_voxel;
}

VoxelBuilder &vdef::create(VoxelType type, const std::string &name)
{
    const auto it = vdef::builders.find(name);

    if(it != vdef::builders.cend()) {
        spdlog::warn("vdef: {} is created twice", name);
        return it->second;
    }

    return vdef::builders.emplace(name, VoxelBuilder(type, name)).first->second;
}

VoxelInfo *vdef::find(const std::string &name)
{
    for(VoxelInfo &it : vdef::voxels) {
        if(it.name.compare(name))
            continue;
        return &it;
    }

    return nullptr;
}

VoxelInfo *vdef::find(const Voxel voxel)
{
    if((voxel != NULL_VOXEL) && (voxel <= vdef::voxels.size()))
        return &vdef::voxels.at(voxel - 1);
    return nullptr;
}
