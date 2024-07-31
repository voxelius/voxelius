// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CANVAS_CANVAS_HH
#define CLIENT_CANVAS_CANVAS_HH
#include <client/canvas/font.hh>
#include <client/canvas/text.hh>
#include <shared/math/vec4f.hh>

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
void draw_text(int xpos, int ypos, const Text &text, const Font &font);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, float scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, float scale);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg);
void draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg, float scale);
} // namespace canvas

#endif /* CLIENT_CANVAS_CANVAS_HH */
