// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_CANVAS_HH
#define CLIENT_UI_CANVAS_HH
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

void rect(int xpos, int ypos, int width, int height);
void rect(int xpos, int ypos, int width, int height, const vector4_t &color);
void rect(int xpos, int ypos, int width, int height, const glxx::Texture2D &texture);
void rect(int xpos, int ypos, int width, int height, const vector4_t &color, const glxx::Texture2D &texture);
void rect_h(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly);
void rect_v(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly);

void text(int xpos, int ypos, const Text &text, const Font &font);
void text(int xpos, int ypos, const Text &text, const Font &font, double scale);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, double scale);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg);
void text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg, double scale);
} // namespace canvas

#endif /* CLIENT_UI_CANVAS_HH */
