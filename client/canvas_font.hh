// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_FONT_HH
#define CLIENT_FONT_HH
#include <client/glxx/texture.hh>
#include <shared/vfs.hh>

namespace canvas
{
class Font final {
public:
    bool load(const vfs::path_t &path, int width, int height);
    void unload();

    int get_glyph_width() const;
    int get_glyph_height() const;
    int get_texture_cwidth() const;
    int get_texture_cheight() const;
    const glxx::Texture2D &get() const;

private:
    int glyph_width {};
    int glyph_height {};
    int texture_cwidth {};
    int texture_cheight {};
    glxx::Texture2D texture {};
};
} // namespace canvas

#endif /* CLIENT_FONT_HH */
