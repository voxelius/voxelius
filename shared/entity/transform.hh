// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <shared/types.hh>

struct TransformComponent final {
    vector3d_t position {};
    vector3d_t angles {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
