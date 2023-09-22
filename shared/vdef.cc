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

static std::unordered_map<std::string, voxel_id_t> name_map = {};
static vdef_vector voxels = {};

void vdef::purge()
{
    for(const auto it : voxels)
        delete it;
    name_map.clear();
    voxels.clear();
}

voxel_id_t vdef::assign(const std::string &name, voxel_id_t hint)
{
    if(const auto nit = name_map.find(name); nit != name_map.cend()) {
        spdlog::error("vdef: name {} is taken by voxel {}", nit->first, nit->second);
        return NULL_VOXEL_ID;
    }

    while(voxels[hint]) {
        if(hint == NULL_VOXEL_ID)
            ++hint;
        spdlog::error("vdef: voxel {} is already assigned", hint);
        ++hint;
    }

    const std::string path1 = fmt::format("/voxels/{}.json", name);
    const std::string path2 = fmt::format("/voxels/{}.jsonc", name);

    std::string source;
    if(!filetools::read(path1, source)) {
        if(!filetools::read(path2, source)) {
            spdlog::error("vdef: {}/jsonc: {}", path1, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return NULL_VOXEL_ID;
        }
    }

    JSON_Value *json_val = json_parse_string_with_comments(source.c_str());
    JSON_Object *json = json_value_get_object(json_val);

    if(!json_val) {
        spdlog::error("vdef: {}/jsonc: parse error", path1);
        return NULL_VOXEL_ID;
    }

    if(!json) {
        json_value_free(json_val);
        spdlog::error("vdef: {}/jsonc: root is not an object", path1);
        return NULL_VOXEL_ID;
    }


}

VoxelInfo *vdef::find(const std::string &name)
{
    const auto it = name_map.find(name);
    if(it != name_map.cend())
        return voxels[it->second];
    return nullptr;
}

VoxelInfo *vdef::find(voxel_id_t id)
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
