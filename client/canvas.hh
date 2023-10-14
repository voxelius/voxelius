// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_CANVAS_HH
#define CLIENT_CANVAS_HH
#include <client/glxx/texture.hh>
#include <shared/const.hh>

namespace canvas
{
class Font;
class Text;
} // namespace canvas

namespace canvas
{
void init();
void deinit();
void prepare();

void draw_rect(int xpos, int ypos, int width, int height);
void draw_rect(int xpos, int ypos, int width, int height, const glm::dvec4 &color);
void draw_rect(int xpos, int ypos, int width, int height, const glxx::Texture2D &texture);
void draw_rect(int xpos, int ypos, int width, int height, const glm::dvec4 &color, const glxx::Texture2D &texture);
void draw_rect_h(int xpos, int ypos, int width, int height, const glm::dvec4 &colx, const glm::dvec4 &coly);
void draw_rect_v(int xpos, int ypos, int width, int height, const glm::dvec4 &colx, const glm::dvec4 &coly);

void draw_text(int xpos, int ypos, const Text &text, const Font &font);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, unsigned int scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const glm::dvec4 &fg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const glm::dvec4 &fg, unsigned int scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const glm::dvec4 &fg, const glm::dvec4 &bg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const glm::dvec4 &fg, const glm::dvec4 &bg, unsigned int scale);
} // namespace canvas

#endif /* CLIENT_CANVAS_HH */
