// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/vec4f.hh>

float Vec4f::length(const Vec4f &vector)
{
    return std::sqrt(Vec4f::dot(vector, vector));
}

float Vec4f::length2(const Vec4f &vector)
{
    return Vec4f::dot(vector, vector);
}

float Vec4f::normalize(Vec4f &vector)
{
    const float retval = Vec4f::length(vector);
    const float multip = 1.0f / retval;
    vector[0] *= multip;
    vector[1] *= multip;
    vector[2] *= multip;
    vector[3] *= multip;
    return retval;
}
