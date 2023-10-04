// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_LABEL_HH
#define CLIENT_UI_LABEL_HH
#include <client/ui/font.hh>
#include <shared/types.hh>
#include <string>

namespace ui
{
class Label final {
public:
    void set_text(const std::wstring &text);
    void set_center(const vector2i_t &center);
    void set_color(const vector4_t &color);
    void set_scale(const vector2_t &scale);
    void destroy();

private:
    int texture_width {};
    vector2i_t center {};
    vector4_t color {1.0, 1.0, 1.0, 1.0};
    vector2_t scale {1.0, 1.0};
    gl::Texture2D texture {};

public:
    static void init();
    static void deinit();
    static void draw(const Label &label, const Font &font);
};
} // namespace ui

#endif/* CLIENT_UI_LABEL_HH */
