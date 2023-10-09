// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_IMMEDIATE_HH
#define CLIENT_UI_IMMEDIATE_HH
#include <shared/types.hh>

namespace ui
{
class Font;
class Text;
} // namespace ui

namespace ui::im
{
constexpr static const unsigned int BUTTON_IDLE = 0U;
constexpr static const unsigned int BUTTON_HOVER = 1U;
constexpr static const unsigned int BUTTON_PRESS = 2U;
constexpr static const unsigned int BUTTON_STATES = 3U;

struct Style final {
    unsigned int element_scale {1U};
    vector4_t button_background[BUTTON_STATES] {};
    vector4_t button_foreground[BUTTON_STATES] {};
    vector2i_t button_margin {0, 0};
};
} // namespace ui::im

namespace ui::im
{
void init();
void update_late();
bool button(int xpos, int ypos, const Text &text, const Font &font, const Style &style);
} // namespace ui::im

#endif/* CLIENT_UI_IMMEDIATE_HH */
