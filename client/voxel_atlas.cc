// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/image.hh>
#include <client/voxel_atlas.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

static int atlas_width = 0;
static int atlas_height = 0;
static size_t atlas_count = 0;
static glxx::Texture2DArray atlas_obj = {};
static std::vector<AtlasTexture> atlas_vec = {};
static std::unordered_map<vfs::path_t, size_t, vfs::path_hasher_t> atlas_map = {};

void voxel_atlas::create(int width, int height, size_t count)
{
    voxel_atlas::destroy();

    atlas_width = width;
    atlas_height = height;
    atlas_count = count;

    atlas_obj.create();
    atlas_obj.storage(width, height, static_cast<int>(count), Image::RGBA);
}

void voxel_atlas::generate_mipmap()
{
    atlas_obj.generate_mipmap();
}

void voxel_atlas::destroy()
{
    atlas_width = 0;
    atlas_height = 0;
    atlas_count = 0;
    atlas_obj.destroy();
    atlas_vec.clear();
    atlas_map.clear();
}

const AtlasTexture *voxel_atlas::load(const vfs::path_t &path)
{
    if(const auto it = atlas_map.find(path); it != atlas_map.cend()) {
        return &atlas_vec[it->second];
    }

    if(atlas_vec.size() + 1 > atlas_count) {
        spdlog::error("atlas: size exceeded [{}/{}]", atlas_vec.size() + 1, atlas_count);
        return nullptr;
    }

    Image image = {};

    if(image.load_rgba(path, true)) {
        const int iwidth = image.get_width();
        const int iheight = image.get_height();

        if(iwidth > atlas_width || iheight > atlas_height) {
            // UNDONE: bring std_image_resize here?
            spdlog::error("atlas: {}: texture too big", path.string());
            return nullptr;
        }

        AtlasTexture texture = {};
        texture.texture = atlas_vec.size();

        atlas_obj.write(texture.texture, 0, 0, iwidth, iheight, Image::RGBA, image.data());

        atlas_map.emplace(path, texture.texture);
        atlas_vec.push_back(texture);

        return &atlas_vec[texture.texture];
    }

    spdlog::error("atlas: {}: load error", path.string());
    return nullptr;
}

const AtlasTexture *voxel_atlas::find(const vfs::path_t &path)
{
    if(const auto it = atlas_map.find(path); it != atlas_map.cend())
        return &atlas_vec[it->second];
    return nullptr;
}

const AtlasTexture *voxel_atlas::find(const uint32_t tex_id)
{
    if(tex_id < atlas_vec.size())
        return &atlas_vec[tex_id];
    return nullptr;
}

const glxx::Texture2DArray &voxel_atlas::get()
{
    return atlas_obj;
}
