// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_DRAW_HH
#define CLIENT_UI_DRAW_HH
#include <client/ui_font.hh>
#include <client/ui_label.hh>
#include <client/ui_rect.hh>

namespace ui
{
void init();
void deinit();
void draw(const ui::Rect &rect);
void draw(const ui::Rect &rect, const gl::Texture2D &texture);
void draw(const ui::Label &label, const ui::Font *font, const ui::Rect &rect);
} // namespace ui

#endif/* CLIENT_UI_DRAW_HH */
