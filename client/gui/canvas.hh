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
void draw_rect(int xpos, int ypos, int width, int height, GLuint texture);
void draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color);
void draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color, GLuint texture);
void draw_rect_h(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly);
void draw_rect_v(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly);
} // namespace canvas

namespace canvas
{
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fgmod);
void draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fgmod, const Vec4f &bgmod);
} // namespace canvas

#endif /* CLIENT_GUI_CANVAS_HH */
