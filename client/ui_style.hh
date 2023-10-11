// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_STYLE_HH
#define CLIENT_UI_STYLE_HH
#include <shared/color.hh>

namespace ui
{
struct LabelStyle final {
    vector4_t background {COL_TRANSPARENT};
    vector4_t foreground {COL_WHITE};
    vector4_t shadow {COL_TRANSPARENT};
};

struct ButtonStyle final {
    vector4_t background_default {COL_TRANSPARENT};
    vector4_t background_hovered {COL_TRANSPARENT};
    vector4_t background_pressed {COL_TRANSPARENT};
    vector4_t foreground_default {COL_WHITE};
    vector4_t foreground_hovered {COL_WHITE};
    vector4_t foreground_pressed {COL_WHITE};
    vector4_t text_shadow {COL_TRANSPARENT};
    vector2i_t text_border {0, 0};
};

struct SliderStyle final {
    vector4_t background_default {COL_TRANSPARENT};
    vector4_t background_hovered {COL_TRANSPARENT};
    vector4_t foreground_default {COL_WHITE};
    vector4_t foreground_hovered {COL_WHITE};
    vector4_t foreground_pressed {COL_WHITE};
    vector4_t text_color {COL_WHITE};
    vector2i_t text_border {0, 0};
};

struct Style final {
    LabelStyle label {};
    ButtonStyle button {};
    SliderStyle slider {};
};
} // namespace ui

#endif /* CLIENT_UI_STYLE_HH */
