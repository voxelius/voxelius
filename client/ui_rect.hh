// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_RECT_HH
#define CLIENT_UI_RECT_HH
#include <shared/types.hh>

namespace ui
{
struct Rect final {
    vector2f_t position {};
    vector2f_t size {1.0, 1.0};
    vector4f_t color {1.0, 1.0, 1.0, 1.0};
};
} // namespace ui

#endif/* CLIENT_UI_RECT_HH */
