// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/image.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

static int atlas_width = 0;
static int atlas_height = 0;
static size_t atlas_count = 0;
static glxx::Texture2DArray atlas_obj = {};
static std::vector<AtlasTexture> atlas_vec = {};
static std::unordered_map<std::string, size_t> atlas_map = {};

void atlas::create(int width, int height, size_t count)
{
    atlas::destroy();

    atlas_width = width;
    atlas_height = height;
    atlas_count = count;

    atlas_obj.create();
    atlas_obj.storage(width, height, static_cast<int>(count), Image::FORMAT);
}

void atlas::destroy()
{
    atlas_width = 0;
    atlas_height = 0;
    atlas_count = 0;
    atlas_obj.destroy();
    atlas_vec.clear();
    atlas_map.clear();
}

void atlas::generate_mipmap()
{
    atlas_obj.generate_mipmap();
}

const AtlasTexture *atlas::emplace(const std::string &path)
{
    if(const auto it = atlas_map.find(path); it != atlas_map.cend()) {
        return &atlas_vec[it->second];
    }

    if(atlas_vec.size() + 1 >= atlas_count) {
        spdlog::error("atlas: size exceeded [{}/{}]", atlas_vec.size() + 1, atlas_count);
        return nullptr;
    }

    Image image = {};

    if(image.create(path)) {
        int iwidth, iheight;
        image.get_size(iwidth, iheight);

        if(iwidth > atlas_width || iheight > atlas_height) {
            // UNDONE: bring std_image_resize here?
            spdlog::error("atlas: {}: texture too big", path);
            return nullptr;
        }

        AtlasTexture texture = {};
        texture.texture = atlas_vec.size();
        texture.max_uv.x = static_cast<float>(iwidth) / static_cast<float>(atlas_width);
        texture.max_uv.y = static_cast<float>(iheight) / static_cast<float>(atlas_height);

        atlas_obj.write(texture.texture, 0, 0, iwidth, iheight, Image::FORMAT, image.data());

        atlas_map.emplace(path, texture.texture);
        atlas_vec.push_back(texture);

        return &atlas_vec[texture.texture];
    }

    return nullptr;
}

const AtlasTexture *atlas::find(const std::string &path)
{
    if(const auto it = atlas_map.find(path); it != atlas_map.cend())
        return &atlas_vec[it->second];
    return nullptr;
}

const AtlasTexture *atlas::find(const uint32_t texture)
{
    if(texture < atlas_vec.size())
        return &atlas_vec[texture];
    return nullptr;
}

const glxx::Texture2DArray &atlas::get()
{
    return atlas_obj;
}
