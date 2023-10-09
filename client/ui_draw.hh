// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_DRAW_HH
#define CLIENT_UI_DRAW_HH
#include <client/gl_texture.hh>
#include <shared/types.hh>

namespace ui
{
class Font;
class Text;
} // namespace ui

namespace ui::draw
{
void init();
void deinit();

void rect(int xpos, int ypos, int width, int height);
void rect(int xpos, int ypos, int width, int height, const vector4_t &color);
void rect(int xpos, int ypos, int width, int height, const gl::Texture2D &texture);
void rect(int xpos, int ypos, int width, int height, const vector4_t &color, const gl::Texture2D &texture);

void text(int xpos, int ypos, const Text &text, const Font &font);
void text(int xpos, int ypos, const Text &text, const Font &font, unsigned int scale);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, unsigned int scale);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg, unsigned int scale);
} // namespace ui::draw

#endif/* CLIENT_UI_DRAW_HH */
