// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_EVENT_KEYBOARD_KEY_HH
#define CLIENT_EVENT_KEYBOARD_KEY_HH
#include <GLFW/glfw3.h>

struct KeyEvent final {
    int key {GLFW_KEY_UNKNOWN};
    int scancode {};
    int action {};
    int mods {};
};

#endif /* CLIENT_EVENT_KEYBOARD_KEY_HH */
