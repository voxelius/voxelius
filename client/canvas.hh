// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_CANVAS_HH
#define CLIENT_CANVAS_HH
#include <client/glxx/texture.hh>
#include <shared/types.hh>

namespace canvas
{
class Font;
class Text;
} // namespace canvas

namespace canvas
{
void init();
void deinit();

void draw_rect(int xpos, int ypos, int width, int height);
void draw_rect(int xpos, int ypos, int width, int height, const vector4d_t &color);
void draw_rect(int xpos, int ypos, int width, int height, const glxx::Texture2D &texture);
void draw_rect(int xpos, int ypos, int width, int height, const vector4d_t &color, const glxx::Texture2D &texture);
void draw_rect_h(int xpos, int ypos, int width, int height, const vector4d_t &colx, const vector4d_t &coly);
void draw_rect_v(int xpos, int ypos, int width, int height, const vector4d_t &colx, const vector4d_t &coly);

void draw_text(int xpos, int ypos, const Text &text, const Font &font);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, unsigned int scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const vector4d_t &fg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const vector4d_t &fg, unsigned int scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const vector4d_t &fg, const vector4d_t &bg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const vector4d_t &fg, const vector4d_t &bg, unsigned int scale);
} // namespace canvas

#endif /* CLIENT_CANVAS_HH */
