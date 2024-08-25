// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <mathlib/vec2f.hh>

float Vec2f::length(const Vec2f &vector)
{
    return std::sqrt(Vec2f::dot(vector, vector));
}

float Vec2f::length2(const Vec2f &vector)
{
    return Vec2f::dot(vector, vector);
}

float Vec2f::normalize(Vec2f &vector)
{
    const float retval = Vec2f::length(vector);
    const float multip = 1.0f / retval;
    vector[0] *= multip;
    vector[1] *= multip;
    return retval;
}
