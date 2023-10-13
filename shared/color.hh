// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_COLOR_HH
#define SHARED_COLOR_HH
#include <shared/types.hh>

constexpr static const vector4d_t COL_TRANSPARENT    = {0.00, 0.00, 0.00, 0.00};
constexpr static const vector4d_t COL_BLACK          = {0.00, 0.00, 0.00, 1.00};
constexpr static const vector4d_t COL_BLUE           = {0.00, 0.00, 1.00, 1.00};
constexpr static const vector4d_t COL_GREEN          = {0.00, 1.00, 0.00, 1.00};
constexpr static const vector4d_t COL_CYAN           = {0.00, 1.00, 1.00, 1.00};
constexpr static const vector4d_t COL_RED            = {1.00, 0.00, 0.00, 1.00};
constexpr static const vector4d_t COL_MAGENTA        = {1.00, 0.00, 1.00, 1.00};
constexpr static const vector4d_t COL_YELLOW         = {1.00, 1.00, 0.00, 1.00};
constexpr static const vector4d_t COL_WHITE          = {1.00, 1.00, 1.00, 1.00};
constexpr static const vector4d_t COL_GRAY           = {0.50, 0.50, 0.50, 1.00};
constexpr static const vector4d_t COL_LIGHT_GRAY     = {0.80, 0.80, 0.80, 1.00};
constexpr static const vector4d_t COL_DARK_BLUE      = {0.00, 0.00, 0.50, 1.00};
constexpr static const vector4d_t COL_DARK_GREEN     = {0.00, 0.50, 0.00, 1.00};
constexpr static const vector4d_t COL_DARK_CYAN      = {0.00, 0.50, 0.50, 1.00};
constexpr static const vector4d_t COL_DARK_RED       = {0.50, 0.00, 0.00, 1.00};
constexpr static const vector4d_t COL_DARK_MAGENTA   = {0.50, 0.00, 0.50, 1.00};
constexpr static const vector4d_t COL_BROWN          = {0.50, 0.50, 0.00, 1.00};

#endif /* SHARED_COLOR_HH */
