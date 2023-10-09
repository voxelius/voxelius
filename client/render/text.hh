// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_RENDER_TEXT_HH
#define CLIENT_RENDER_TEXT_HH
#include <client/render/gl/texture.hh>
#include <string>

class Text final {
public:
    void create(int width, int height);
    void set(int line, const std::string &value);
    void set(int line, const std::wstring &value);
    void destroy();

    int get_max_text_width() const;
    int get_max_text_height() const;
    int get_texture_width() const;
    int get_texture_height() const;
    const gl::Texture2D &get() const;

private:
    int max_text_width {};
    int max_text_height {};
    int texture_width {};
    int texture_height {};
    gl::Texture2D texture {};
};

#endif /* CLIENT_RENDER_TEXT_HH */
