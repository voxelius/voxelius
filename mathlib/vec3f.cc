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
    
Vec3f Vec3f::normalized(const Vec3f &vector)
{
    Vec3f result = {};
    const float multip = 1.0f / Vec3f::length(vector);
    result[0] = multip * vector[0];
    result[1] = multip * vector[1];
    result[2] = multip * vector[2];
    return std::move(result);
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
