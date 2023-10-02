// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_INERTIAL_HH
#define SHARED_INERTIAL_HH
#include <shared/world.hh>

namespace inertial
{
void update(World &world, double frametime);
} // namespace inertial

#endif/* SHARED_INERTIAL_HH */
