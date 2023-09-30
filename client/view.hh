// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef AF5EC318_9471_46CE_B408_CBD968EC6F7C
#define AF5EC318_9471_46CE_B408_CBD968EC6F7C
#include <shared/coord.hh>

namespace view
{
void update();
const vector3_t &get_angles();
const chunk_pos_t &get_cpos();
const vector3f_t &get_lpos_float();
const matrix4x4f_t &get_matrix();
} // namespace view

#endif/* AF5EC318_9471_46CE_B408_CBD968EC6F7C */
