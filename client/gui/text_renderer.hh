// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_RENDERER_HH
#define CLIENT_GUI_TEXT_RENDERER_HH

class BitmapFont;
class TextVBO;

namespace text_renderer
{
void init(void);
void deinit(void);
void prepare(void);
void draw(float xpos, float ypos, const BitmapFont &font, const TextVBO &text, float scale);
void draw_shadow(float xpos, float ypos, const BitmapFont &font, const TextVBO &text, float scale);
} // namespace text_renderer

#endif /* CLIENT_GUI_TEXT_RENDERER_HH */
