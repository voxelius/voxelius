// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <parson.h>
#include <shared/util/physfs.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

std::unordered_map<std::string, VDefBuilder> vdef::builders = {};
std::unordered_map<std::string, Voxel> vdef::names = {};
std::vector<VoxelInfo *> vdef::voxels = {};

static VoxelFace parse_face_name(const std::string &face_str)
{
    if(!face_str.compare("north"))
        return VoxelFace::CubeNorth;
    if(!face_str.compare("south"))
        return VoxelFace::CubeSouth;
    if(!face_str.compare("east"))
        return VoxelFace::CubeEast;
    if(!face_str.compare("west"))
        return VoxelFace::CubeWest;
    if(!face_str.compare("top"))
        return VoxelFace::CubeTop;
    if(!face_str.compare("bottom"))
        return VoxelFace::CubeBottom;
    if(!face_str.compare("nesw"))
        return VoxelFace::CrossNESW;
    if(!face_str.compare("nwse"))
        return VoxelFace::CrossNWSE;
    return VoxelFace::Invalid;
}

static bool parse_string_array(const JSON_Array *array, std::vector<std::string> &vector)
{
    vector.resize(json_array_get_count(array));

    for(std::size_t i = 0; i < vector.size(); ++i) {
        if(const char *str = json_array_get_string(array, i)) {
            vector[i] = std::string(str);
            continue;
        }

        return false;
    }

    return true;
}

VDefBuilder::VDefBuilder(const std::string &name, VoxelType type)
{
    this->name = name;
    this->type = type;
}

VDefBuilder &VDefBuilder::add_state(const std::string &state)
{
    for(std::size_t i = 0; i < states.size(); ++i) {
        if(!states[i].compare(state)) {
            spdlog::warn("vdef: {}: state {} is already mapped to offset {}", name, state, i);
            return *this;
        }
    }

    states.push_back(state);
    return *this;
}

VDefBuilder &VDefBuilder::add_default_state(void)
{
    return add_state("default");
}

Voxel VDefBuilder::build(void) const
{
    const auto it = vdef::names.find(name);

    if(it != vdef::names.cend()) {
        spdlog::warn("vdef: cannot build {}: name already registered", name);
        return it->second;
    }

    const std::string json_path = fmt::format("/voxels/{}.json", name);

    std::string json_source = {};

    if(!util::read_string(json_path, json_source)) {
        spdlog::warn("vdef: {}: {}", json_path, util::physfs_error());
        return NULL_VOXEL;
    }

    JSON_Value *jsonv = json_parse_string(json_source.c_str());
    const JSON_Object *json = json_value_get_object(jsonv);

    if(!jsonv) {
        spdlog::warn("vdef: {}: parse error", json_path);
        return NULL_VOXEL;
    }

    if(!json) {
        spdlog::warn("vdef: {}: root is not an object", json_path);
        json_value_free(jsonv);
        return NULL_VOXEL;
    }

    if((vdef::voxels.size() + states.size()) >= VOXEL_MAX) {
        spdlog::critical("vdef: voxel registry overflow");
        json_value_free(jsonv);
        std::terminate();
    }

    std::size_t face_count = {};
    std::vector<VoxelInfo *> infos = {};
    Voxel base = vdef::voxels.size() + 1;

    switch(type) {
        case VoxelType::Cube:
            face_count = static_cast<std::size_t>(VoxelFace::CubeCount);
            break;
        case VoxelType::Slab:
            face_count = static_cast<std::size_t>(VoxelFace::SlabCount);
            break;
        case VoxelType::Stairs:
            face_count = static_cast<std::size_t>(VoxelFace::StairCount);
            break;
        case VoxelType::Cross:
            face_count = static_cast<std::size_t>(VoxelFace::CrossCount);
            break;
        case VoxelType::VModel:
        case VoxelType::DModel:
            // Custom models should use a different texture
            // resource management that is not a voxel atlas
            // TODO: actually implement custom models lol
            face_count = 0;
            break;
        default:
            // Something really bad should happen if we end up here.
            // The outside code would static_cast an int to VoxelType
            // and possibly fuck a lot of things up to cause this
            spdlog::critical("vdef: {}: unknown voxel type {}", name, static_cast<int>(type));
            json_value_free(jsonv);
            std::terminate();
    }

    for(const std::string &state_name : states) {
        VoxelInfoAnimated *info_a = nullptr;
        VoxelInfoVaried *info_v = nullptr;
        VoxelInfo *info = nullptr;

        const JSON_Object *state = json_object_get_object(json, state_name.c_str());

        if(!state) {
            spdlog::warn("vdef: {}: {} is not an object", json_path, state_name);
            json_value_free(jsonv);
            return NULL_VOXEL;
        }

        if(json_object_get_boolean(state, "animated")) {
            info_a = new VoxelInfoAnimated();
            info_a->animated = true;
            info_a->textures.resize(face_count);
            info = info_a;
        }
        else {
            info_v = new VoxelInfoVaried();
            info_v->animated = false;
            info_v->textures.resize(face_count);
            info = info_v;
        }

        info->name = name;
        info->type = type;
        info->base = base;

        info->blending = json_object_get_boolean(state, "blending");

        const JSON_Object *textures = json_object_get_object(state, "textures");
        const std::size_t num_textures = json_object_get_count(textures);

        std::vector<std::string> default_texture = {};

        if(!parse_string_array(json_object_get_array(textures, "default"), default_texture)) {
            spdlog::error("vdef: {}: {}: default: non-string array entry", json_path, state_name);
            json_value_free(jsonv);
            return NULL_VOXEL;
        }

        if(info->animated) {
            for(std::size_t i = 0; i < face_count; ++i) {
                info_a->textures[i].paths = default_texture;
                info_a->textures[i].cached_offset = SIZE_MAX;
            }
        }
        else {
            for(std::size_t i = 0; i < face_count; ++i) {
                info_v->textures[i].paths = default_texture;
                info_v->textures[i].planes.assign(default_texture.size(), SIZE_MAX);
                info_v->textures[i].indices.assign(default_texture.size(), SIZE_MAX);
            }
        }

        for(std::size_t i = 0; i < num_textures; ++i) {
            const char *tex_name_cstr = json_object_get_name(textures, i);
            const std::string tex_name = tex_name_cstr ? tex_name_cstr : "NULL";

            if(!tex_name.compare("default")) {
                // We already parsed the default texture
                // so doing anything else with it makes no sense
                continue;
            }

            const VoxelFace face = parse_face_name(tex_name);
            const std::size_t face_index = static_cast<std::size_t>(face);

            if((face == VoxelFace::Invalid) || (face_index >= face_count)) {
                spdlog::warn("vdef: {}: invalid texture name {}", json_path, state_name, tex_name);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            const JSON_Value *texturev = json_object_get_value_at(textures, i);
            const JSON_Array *texture = json_value_get_array(texturev);

            if(!texture) {
                spdlog::warn("vdef: {}: {}: {} is not an array", json_path, state_name, tex_name);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            std::vector<std::string> paths = {};

            if(!parse_string_array(texture, paths)) {
                spdlog::warn("vdef: {}: {}: {}: non-string array entry", json_path, state_name, tex_name);
                json_value_free(jsonv);
                return NULL_VOXEL;
            }

            if(info->animated) {
                info_a->textures[face_index].paths = paths;
                info_a->textures[face_index].cached_offset = SIZE_MAX;
            }
            else {
                info_v->textures[face_index].paths = paths;
                info_v->textures[face_index].planes.assign(paths.size(), SIZE_MAX);
                info_v->textures[face_index].indices.assign(paths.size(), SIZE_MAX);
            }
        }

        infos.push_back(info);
    }

    json_value_free(jsonv);

    vdef::voxels.insert(vdef::voxels.end(), infos.cbegin(), infos.cend());

    return base;
}

VDefBuilder &vdef::create(const std::string &name, VoxelType type)
{
    const auto it = vdef::builders.find(name);
    if(it != vdef::builders.cend())
        return it->second;
    return vdef::builders.emplace(name, VDefBuilder(name, type)).first->second;
}

VoxelInfo *vdef::find(const std::string &name)
{
    const auto it = vdef::names.find(name);
    if(it != vdef::names.cend())
        return vdef::find(it->second);
    return nullptr;
}

VoxelInfo *vdef::find(const Voxel voxel)
{
    if((voxel != NULL_VOXEL) && (voxel <= vdef::voxels.size()))
        return vdef::voxels[voxel - 1];
    return nullptr;
}

void vdef::purge(void)
{
    for(VoxelInfo *info : vdef::voxels)
        delete info;
    vdef::builders.clear();
    vdef::names.clear();
    vdef::voxels.clear();
}
