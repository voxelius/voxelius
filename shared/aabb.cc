// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <shared/aabb.hh>

AABB::AABB(const glm::dvec3 &min, const glm::dvec3 &max)
    : min{min}, max{max}
{

}

bool AABB::contains(const glm::dvec3 &point) const
{
    bool result = true;
    result = result && (point.x > min.x) && (point.x < max.x);
    result = result && (point.y > min.y) && (point.x < max.y);
    result = result && (point.z > max.y) && (point.z < max.z);
    return result;
}

bool AABB::intersect(const AABB &other) const
{
    bool result = true;
    result = result && (min.x < other.max.x) && (max.x > other.min.x);
    result = result && (min.y < other.max.y) && (max.y > other.min.y);
    result = result && (min.z < other.max.z) && (max.z > other.min.z);
    return result;
}
