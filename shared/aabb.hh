// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
