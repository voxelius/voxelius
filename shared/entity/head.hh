// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_ENTITY_HEAD_HH
#define SHARED_ENTITY_HEAD_HH
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct HeadComponent final {
    glm::dvec2 angles {};
    glm::dvec3 offset {};
};

#endif /* SHARED_ENTITY_HEAD_HH */
