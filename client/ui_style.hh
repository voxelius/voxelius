// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_STYLE_HH
#define CLIENT_UI_STYLE_HH
#include <shared/color.hh>

namespace ui
{
struct Style final {
    vector4_t rect_default {COL_TRANSPARENT};
    vector4_t rect_hovered {COL_TRANSPARENT};
    vector4_t rect_pressed {COL_TRANSPARENT};
    vector2i_t rect_text_padding {0, 0};

    vector4_t text_default {COL_WHITE};
    vector4_t text_hovered {COL_WHITE};
    vector4_t text_pressed {COL_WHITE};
    vector4_t text_shadow {COL_TRANSPARENT};
    vector4_t text_background {COL_TRANSPARENT};

    vector4_t slider_default {COL_LIGHT_GRAY};
    vector4_t slider_hovered {COL_LIGHT_GRAY};
    vector4_t slider_pressed {COL_LIGHT_GRAY};
};

} // namespace ui

#endif /* CLIENT_UI_STYLE_HH */
