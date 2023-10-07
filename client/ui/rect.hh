// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_RECT_HH
#define CLIENT_UI_RECT_HH
#include <client/gl/texture.hh>
#include <shared/types.hh>

namespace ui
{
class Rect final {
public:
    void set_position(const vector2i_t &value);
    void set_color(const vector4_t &value);
    void set_size(const vector2u_t &value);
    void set_scale(unsigned int value);

private:
    vector2i_t position {};
    vector4_t color {1.0, 1.0, 1.0, 1.0};
    vector2u_t size {1U, 1U};
    unsigned int scale {1U};

public:
    static void init();
    static void deinit();
    static void draw(const Rect &rect);
    static void draw(const Rect &rect, const gl::Texture2D &texture);
};
} // namespace ui

#endif /* CLIENT_UI_RECT_HH */

