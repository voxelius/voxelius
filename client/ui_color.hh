// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_UI_COLOR_HH
#define CLIENT_UI_COLOR_HH
#include <shared/types.hh>

namespace ui
{
constexpr static const vector4_t TRANSPARENT    = {0.00, 0.00, 0.00, 0.00};
constexpr static const vector4_t BLACK          = {0.00, 0.00, 0.00, 1.00};
constexpr static const vector4_t BLUE           = {0.00, 0.00, 1.00, 1.00};
constexpr static const vector4_t GREEN          = {0.00, 1.00, 0.00, 1.00};
constexpr static const vector4_t CYAN           = {0.00, 1.00, 1.00, 1.00};
constexpr static const vector4_t RED            = {1.00, 0.00, 0.00, 1.00};
constexpr static const vector4_t MAGENTA        = {1.00, 0.00, 1.00, 1.00};
constexpr static const vector4_t YELLOW         = {1.00, 1.00, 0.00, 1.00};
constexpr static const vector4_t WHITE          = {1.00, 1.00, 1.00, 1.00};
constexpr static const vector4_t GRAY           = {0.50, 0.50, 0.50, 1.00};
constexpr static const vector4_t LIGHT_GRAY     = {0.80, 0.80, 0.80, 1.00};
constexpr static const vector4_t DARK_BLUE      = {0.00, 0.00, 0.50, 1.00};
constexpr static const vector4_t DARK_GREEN     = {0.00, 0.50, 0.00, 1.00};
constexpr static const vector4_t DARK_CYAN      = {0.00, 0.50, 0.50, 1.00};
constexpr static const vector4_t DARK_RED       = {0.50, 0.00, 0.00, 1.00};
constexpr static const vector4_t DARK_MAGENTA   = {0.50, 0.00, 0.50, 1.00};
constexpr static const vector4_t BROWN          = {0.50, 0.50, 0.00, 1.00};
} // namespace ui

#endif /* CLIENT_UI_COLOR_HH */

