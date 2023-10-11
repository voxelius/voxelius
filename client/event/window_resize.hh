// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_EVENT_WINDOW_RESIZE_HH
#define CLIENT_EVENT_WINDOW_RESIZE_HH

struct WindowResizeEvent final {
    int width {};
    int height {};
    double aspect {};
};

#endif /* CLIENT_EVENT_WINDOW_RESIZE_HH */
