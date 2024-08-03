// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_RECT_RENDERER_HH
#define CLIENT_GUI_RECT_RENDERER_HH
#include <shared/math/vec4f.hh>

namespace rect_renderer
{
void init(void);
void deinit(void);
void prepare(void);
void draw_o(float xpos, float ypos, float width, float height, const Vec4f &color);
void draw_s(float xpos, float ypos, float width, float height, const Vec4f &color);
void draw_sh(float xpos, float ypos, float width, float height, const Vec4f &colx, const Vec4f &coly);
void draw_sv(float xpos, float ypos, float width, float height, const Vec4f &colx, const Vec4f &coly);
} // namespace rect_renderer

#endif /* CLIENT_GUI_RECT_RENDERER_HH */
