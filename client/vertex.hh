// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef E4173999_DC44_49FA_882B_7F84EC7381B3
#define E4173999_DC44_49FA_882B_7F84EC7381B3
#include <shared/types.hh>

struct Vertex final {
    vec3f_t position {};
    vec3f_t normal {};
    vec2f_t texcoord {};
    uint32_t texture {};
};

#endif/* E4173999_DC44_49FA_882B_7F84EC7381B3 */
