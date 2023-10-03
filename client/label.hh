// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_LABEL_HH
#define CLIENT_LABEL_HH
#include <client/fonts.hh>
#include <shared/types.hh>
#include <string>

class Label final {
public:
    void set_text(const std::wstring &text);
    void set_color(const vector4f_t &color);
    void set_position(const vector2u_t &position);
    void set_scale(const vector2f_t &scale);
    void destroy();

private:
    unsigned int text_size {};
    vector4f_t color {1.0, 1.0, 1.0, 1.0};
    vector2u_t position {};
    vector2f_t scale {1.0, 1.0};
    gl::Texture2D texture {};

public:
    static void init();
    static void deinit();
    static void draw(const Label &label, const Font *font);
};

#endif/* CLIENT_LABEL_HH */
