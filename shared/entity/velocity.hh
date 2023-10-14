// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_ENTITY_VELOCITY_HH
#define SHARED_ENTITY_VELOCITY_HH
#include <glm/vec3.hpp>

struct VelocityComponent final {
    glm::dvec3 angular {};
    glm::dvec3 linear {};
};

#endif /* SHARED_ENTITY_VELOCITY_HH */
