// SPDX-License-Identifier: Zlib
#include <emhash/hash_table8.hpp>
#include <physfs.h>
#include <shared/image.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stdint.h>
#include <vector>

static emhash8::HashMap<std::string, Image> images = {};

const Image *detail::image::load(const std::string &path)
{
    if(const auto it = images.find(path); it != images.cend()) {
        it->second.ref_count += 1UL;
        return &it->second;
    }

    Image skel = {};
    skel.ref_count = 1UL;
    skel.path = path;

    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        std::vector<uint8_t> buffer = {};

        buffer.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, buffer.data(), buffer.size());
        PHYSFS_close(file);

        Image skel = {};
        skel.ref_count = 1UL;
        skel.path = path;

        stbi_set_flip_vertically_on_load(true);
        skel.pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &skel.size.x, &skel.size.y, nullptr, STBI_rgb_alpha);

        if(!skel.pixels) {
            spdlog::warn("image: {}: load failed", path);
            return nullptr;
        }

        return &images.emplace(path, skel).first->second;
    }

    spdlog::warn("image: {}: {}", path, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    return nullptr;
}

const Image *detail::image::find(const std::string &path)
{
    if(const auto it = images.find(path); it != images.cend()) {
        it->second.ref_count += 1UL;
        return &it->second;
    }

    return nullptr;
}

bool detail::image::release(const Image *object)
{
    if(!object) {
        // Object not freed;
        // Argument is a null pointer!
        return false;
    }

    const auto it = images.find(object->path);

    if((it == images.cend()) || (object != &it->second)) {
        // Object not freed;
        // We don't own the object!
        return false;
    }

    it->second.ref_count -= 1UL;

    if(!it->second.ref_count) {
        stbi_image_free(const_cast<void *>(it->second.pixels));
        images.erase(it);
        return true;
    }

    // Object not freed;
    // Someone is using it!
    return false;
}

void detail::image::purge(void)
{
    for(const auto it : images)
        stbi_image_free(const_cast<void *>(it.second.pixels));
    images.clear();
}
