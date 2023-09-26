// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B3D18FD3_A55D_4E01_8C67_46D9B1D6D1E0
#define B3D18FD3_A55D_4E01_8C67_46D9B1D6D1E0
#include <client/glxx/pixel_format.hh>
#include <core/mixin.hh>
#include <core/vfs.hh>
#include <string>

class Image final : public mixin::NonCopyable {
public:
    constexpr static const glxx::PixelFormat FORMAT = glxx::PixelFormat::R8G8B8_UNORM;

public:
    Image() = default;
    Image(const vfs::path_t &path, bool flip = false);
    Image(Image &&rhs);
    virtual ~Image();

    Image &operator=(Image &&rhs);

    bool create(const vfs::path_t &path, bool flip = false);
    void destroy();
    bool valid() const;

    int get_width() const;
    int get_height() const;
    void get_size(int &width, int &height) const;
    const void *data() const;
    void *data();

private:
    int width {};
    int height {};
    void *pixels {nullptr};
};

#endif/* B3D18FD3_A55D_4E01_8C67_46D9B1D6D1E0 */
