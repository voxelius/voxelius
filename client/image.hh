// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_IMAGE_HH
#define CLIENT_IMAGE_HH
#include <client/pixel_format.hh>
#include <shared/mixin.hh>
#include <shared/vfs.hh>
#include <string>

class Image final : public mixin::NonCopyable {
public:
    constexpr static const PixelFormat GRAYSCALE = PixelFormat::R8_UNORM;
    constexpr static const PixelFormat RGBA = PixelFormat::R8G8B8A8_UNORM;

public:
    virtual ~Image();
    bool load_grayscale(const vfs::path_t &path, bool flip);
    bool load_rgba(const vfs::path_t &path, bool flip);
    void unload();

    bool valid() const;
    int get_width() const;
    int get_height() const;
    const void *data() const;
    void *data();

private:
    int width {};
    int height {};
    void *pixels {nullptr};
};

#endif/* CLIENT_IMAGE_HH */
