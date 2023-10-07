// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_LABEL_HH
#define CLIENT_UI_LABEL_HH
#include <client/ui/font.hh>
#include <shared/types.hh>

namespace ui
{
class Label final {
public:
    void create(int width, int height);
    void destroy();

    void set_text(int line, const std::string &text);
    void set_text(int line, const std::wstring &text);
    void set_background(const vector4_t &value);
    void set_foreground(const vector4_t &value);
    void set_position(const vector2i_t &value);
    void set_scale(unsigned int value);

private:
    int width {};
    int height {};
    gl::Texture2D texture {};
    vector4_t background {0.0, 0.0, 0.0, 0.0};
    vector4_t foreground {1.0, 1.0, 1.0, 1.0};
    vector2i_t position {0, 0};
    unsigned int scale {1U};

public:
    static void init();
    static void deinit();
    static void draw(const Label &label, const Font &font);
};
} // namespace ui

#endif /* CLIENT_UI_LABEL_HH */

