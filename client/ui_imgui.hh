// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_IMGUI_HH
#define CLIENT_UI_IMGUI_HH
#include <client/ui_style.hh>
#include <string>

namespace canvas
{
class Font;
class Text;
} // namespace canvas

namespace ui::imgui
{
void init();
void update_late();
} // namespace ui::imgui

namespace ui::imgui
{
void label(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const Style &style, unsigned int scale);
bool button(int xpos, int ypos, int width, const canvas::Text &text, const canvas::Font &font, const Style &style);
void slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style);
void slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max);
void slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max, double step);
} // namespace ui::imgui

#endif /* CLIENT_UI_IMGUI_HH */
