// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_IMGUI_HH
#define CLIENT_UI_IMGUI_HH
#include <shared/color.hh>

namespace canvas
{
class Font;
class Text;
} // namespace canvas

namespace imgui
{
struct Style final {
    vector4_t button_background {COL_TRANSPARENT};
    vector4_t button_background_hover {COL_TRANSPARENT};
    vector4_t button_background_press {COL_TRANSPARENT};
    vector4_t button_foreground {COL_WHITE};
    vector4_t button_foreground_hover {COL_WHITE};
    vector4_t button_foreground_press {COL_WHITE};
    vector2i_t button_margin {0, 0};
};
} // namespace imgui

namespace imgui
{
void init();
void update_late();
bool button(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const Style &style);
} // namespace imgui

#endif /* CLIENT_UI_IMGUI_HH */
