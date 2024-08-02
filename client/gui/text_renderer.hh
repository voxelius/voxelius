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
void draw(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, float scale = 1.0f, float tint = 1.0f);
} // namespace text_renderer

#endif /* CLIENT_GUI_TEXT_RENDERER_HH */
