// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_STYLE_HH
#define CLIENT_UI_STYLE_HH
#include <shared/const.hh>

namespace ui
{
struct Style final {
    glm::dvec4 rect_default {COLOR_TRANSPARENT};
    glm::dvec4 rect_hovered {COLOR_TRANSPARENT};
    glm::dvec4 rect_pressed {COLOR_TRANSPARENT};
    glm::ivec2 rect_text_padding {0, 0};

    glm::dvec4 text_default {COLOR_WHITE};
    glm::dvec4 text_hovered {COLOR_WHITE};
    glm::dvec4 text_pressed {COLOR_WHITE};
    glm::dvec4 text_shadow {COLOR_TRANSPARENT};
    glm::dvec4 text_background {COLOR_TRANSPARENT};

    glm::dvec4 slider_default {COLOR_LIGHT_GRAY};
    glm::dvec4 slider_hovered {COLOR_LIGHT_GRAY};
    glm::dvec4 slider_pressed {COLOR_LIGHT_GRAY};
};

} // namespace ui

#endif /* CLIENT_UI_STYLE_HH */
