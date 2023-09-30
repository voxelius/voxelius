// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B82DAA50_894A_47D9_989E_833EC4C50B89
#define B82DAA50_894A_47D9_989E_833EC4C50B89
#include <GLFW/glfw3.h>

struct KeyEvent final {
    int key {GLFW_KEY_UNKNOWN};
    int scancode {};
    int action {};
    int mods {};
};

struct MouseButtonEvent final {
    int button {GLFW_KEY_UNKNOWN};
    int action {};
    int mods {};
};

struct CursorPosEvent final {
    double xpos {};
    double ypos {};
};

struct ScrollEvent final {
    double dx {};
    double dy {};
};

namespace input
{
void init();
} // namespace input

#endif /* B82DAA50_894A_47D9_989E_833EC4C50B89 */
