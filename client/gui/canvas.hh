// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_CANVAS_HH
#define CLIENT_GUI_CANVAS_HH
#include <glad/gl.h>
#include <shared/math/vec4f.hh>

class BitmapFont;
class TextVBO;

namespace canvas
{
void init(void);
void deinit(void);
void prepare(void);
} // namespace canvas

namespace canvas
{
void draw_rect(int xpos, int ypos, int width, int height);
void draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color);
void draw_rect_h(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly);
void draw_rect_v(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly);
} // namespace canvas

namespace canvas
{
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, float scale);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, float scale);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, const Vec4f &bg);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, const Vec4f &bg, float scale);
} // namespace canvas

#endif /* CLIENT_GUI_CANVAS_HH */
