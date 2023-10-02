// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_SCREEN_HH
#define CLIENT_SCREEN_HH

struct ScreenSizeEvent final {
    int width {};
    int height {};
    double aspect {1.0};
};

namespace screen
{
void init();
void init_late();
void get_size(int &width, int &height);
void get_size(double &width, double &height);
double get_aspect();
} // namespace screen

#endif /* CLIENT_SCREEN_HH */
