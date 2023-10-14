// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_ENTITY_TRANSFORM_HH
#define SHARED_ENTITY_TRANSFORM_HH
#include <glm/vec3.hpp>

struct TransformComponent final {
    glm::dvec3 angles {};
    glm::dvec3 position {};
};

#endif /* SHARED_ENTITY_TRANSFORM_HH */
