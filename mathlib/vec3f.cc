// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <mathlib/vec3f.hh>

float Vec3f::length(const Vec3f &vector)
{
    return std::sqrt(Vec3f::dot(vector, vector));
}

float Vec3f::length2(const Vec3f &vector)
{
    return Vec3f::dot(vector, vector);
}

float Vec3f::normalize(Vec3f &vector)
{
    const float retval = Vec3f::length(vector);
    const float multip = 1.0f / retval;
    vector[0] *= multip;
    vector[1] *= multip;
    vector[2] *= multip;
    return retval;
}
