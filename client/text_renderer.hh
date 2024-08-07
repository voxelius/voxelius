// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_TEXT_RENDERER_HH
#define CLIENT_TEXT_RENDERER_HH

class Font;
class TextVBO;

namespace text_renderer
{
void init(void);
void deinit(void);
void prepare(void);
void draw(float xpos, float ypos, const Font &font, const TextVBO &text, float scale, bool shadow);
} // namespace text_renderer

#endif /* CLIENT_TEXT_RENDERER_HH */
