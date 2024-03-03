// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
