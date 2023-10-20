// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_AABB_HH
#define SHARED_AABB_HH
#include <glm/vec3.hpp>

class AABB final {
public:
    AABB() = default;
    AABB(const glm::dvec3 &min, const glm::dvec3 &max);
    bool contains(const glm::dvec3 &point) const;
    bool intersect(const AABB &other) const;

public:
    glm::dvec3 min {};
    glm::dvec3 max {};
};

#endif /* SHARED_AABB_HH */
