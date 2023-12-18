// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_EVENT_IMGUI_FONTS_RELOAD_HH
#define CLIENT_EVENT_IMGUI_FONTS_RELOAD_HH
#include <imgui.h>

struct ImGuiFontsReloadEvent final {
    ImFontAtlas *fonts {nullptr};
    ImFontConfig config {};
    unsigned int scale {};
};

#endif /* CLIENT_EVENT_IMGUI_FONTS_RELOAD_HH */
