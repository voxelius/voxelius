// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B161F956_A082_4B9E_AD07_9417FE24D22A
#define B161F956_A082_4B9E_AD07_9417FE24D22A

struct ScreenSizeEvent final {
    int width {};
    int height {};
    float aspect {1.0f};
};

namespace screen
{
void init();
void init_late();
void get_size(int &width, int &height);
void get_size(float &width, float &height);
float get_aspect();
} // namespace screen

#endif /* B161F956_A082_4B9E_AD07_9417FE24D22A */
