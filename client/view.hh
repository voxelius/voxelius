// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_VIEW_HH
#define CLIENT_VIEW_HH
#include <shared/coord.hh>

struct ViewValues final {
    vector3_t angles {};
    vector3_t position {};
    vector3_t direction {};
    vector3f_t lpos_float {};
    matrix4x4f_t matrix {};
    chunk_pos_t cpos {};
};

namespace view
{
void update();
const vector3_t &get_angles();
const vector3_t &get_position();
const vector3_t &get_direction();
const vector3f_t &get_lpos_float();
const matrix4x4f_t &get_matrix();
const chunk_pos_t &get_cpos();
void get_values(ViewValues &vv);
} // namespace view

#endif/* CLIENT_VIEW_HH */
