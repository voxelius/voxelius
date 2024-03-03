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
#ifndef CLIENT_IMAGE_HH
#define CLIENT_IMAGE_HH
#include <client/pixel_format.hh>
#include <shared/mixin.hh>
#include <shared/vfs.hh>

class Image final : public NonCopyable {
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

#endif /* CLIENT_IMAGE_HH */
