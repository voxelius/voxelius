// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_FONTS_HH
#define CLIENT_FONTS_HH
#include <client/gl_texture.hh>
#include <shared/vfs.hh>

struct Font final {
    int glyph_width {};
    int glyph_height {};
    int texture_cwidth {}; // IN CHARACTERS
    int texture_cheight {}; // IN CHARACTERS
    gl::Texture2D texture {};
};

namespace fonts
{
const Font *load_image(const std::string &name, const vfs::path_t &path, int width, int height);
const Font *load_rom(const std::string &name, const vfs::path_t &path, int width, int height);
const Font *load_psf(const std::string &name, const vfs::path_t &path, bool unicode);
const Font *find(const std::string &name);
void purge();
} // namespace fonts

#endif/* CLIENT_FONTS_HH */
