// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_DRAW_HH
#define CLIENT_UI_DRAW_HH
#include <client/ui_font.hh>
#include <client/ui_label.hh>

namespace ui
{
void init();
void deinit();
void draw_rect(int xpos, int ypos, int width, int height, const vector4_t &color = {1.0, 1.0, 1.0, 1.0});
void draw_rect(int xpos, int ypos, int width, int height, const gl::Texture2D &texture, const vector4_t &color = {1.0, 1.0, 1.0, 1.0});
void draw_label(int xpos, int ypos, const ui::Label &label, const ui::Font *font, const vector2_t &scale = {1.0, 1.0}, const vector4_t &color = {1.0, 1.0, 1.0, 1.0});
} // namespace ui

#endif/* CLIENT_UI_DRAW_HH */
