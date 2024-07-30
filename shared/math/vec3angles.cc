// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/vec3angles.hh>

Vec3angles Vec3angles::wrap_180(const Vec3angles &angles)
{
    constexpr float A180 = cxpr::radians(180.0f);
    constexpr float A360 = cxpr::radians(360.0f);

    Vec3angles result = {};
    result[0] = std::fmod(angles[0] + A180, A360);
    result[1] = std::fmod(angles[1] + A180, A360);
    result[2] = std::fmod(angles[2] + A180, A360);

    result[0] = ((result[0] < 0.0f) ? (result[0] + A360) : result[0]) - A180;
    result[1] = ((result[1] < 0.0f) ? (result[1] + A360) : result[1]) - A180;
    result[2] = ((result[2] < 0.0f) ? (result[2] + A360) : result[2]) - A180;

    return std::move(result);
}

Vec3angles Vec3angles::wrap_360(const Vec3angles &angles)
{
    constexpr float A360 = cxpr::radians(360.0f);

    Vec3angles result = {};
    result[0] = std::fmod(std::fmod(angles[0], A360) + A360, A360);
    result[1] = std::fmod(std::fmod(angles[1], A360) + A360, A360);
    result[2] = std::fmod(std::fmod(angles[2], A360) + A360, A360);
    return std::move(result);
}

Vec3f Vec3angles::cos(const Vec3angles &angles)
{
    Vec3f result = {};
    result[0] = std::cos(angles[0]);
    result[1] = std::cos(angles[1]);
    result[2] = std::cos(angles[2]);
    return std::move(result);
}

Vec3f Vec3angles::sin(const Vec3angles &angles)
{
    Vec3f result = {};
    result[0] = std::sin(angles[0]);
    result[1] = std::sin(angles[1]);
    result[2] = std::sin(angles[2]);
    return std::move(result);
}

void Vec3angles::vectors(const Vec3angles &angles, Vec3f &forward)
{
    const float cosp = std::cos(angles[0]);
    const float cosy = std::cos(angles[1]);
    const float sinp = std::sin(angles[0]);
    const float siny = std::sin(angles[1]);

    forward[0] = cosp * siny * (-1.0f);
    forward[1] = sinp;
    forward[2] = cosp * cosy * (-1.0f);
}

void Vec3angles::vectors(const Vec3angles &angles, Vec3f *forward, Vec3f *right, Vec3f *up)
{
    if(!forward && !right && !up) {
        // There's no point in figuring out
        // direction vectors if nothing is passed
        // in the function to store that stuff in
        return;
    }

    const Vec3f pcv = Vec3angles::cos(angles);
    const Vec3f psv = Vec3angles::sin(angles);
    const Vec3f ncv = pcv * (-1.0f);
    const Vec3f nsv = psv * (-1.0f);

    if(forward) {
        forward[0][0] = pcv[0] * nsv[1];
        forward[0][1] = psv[0];
        forward[0][2] = pcv[0] * ncv[1];
    }

    if(right) {
        right[0][0] = pcv[2] * pcv[1];
        right[0][1] = psv[2] * pcv[1];
        right[0][2] = nsv[1];
    }

    if(up) {
        up[0][0] = psv[0] * psv[1] * pcv[2] + ncv[1] * psv[2];
        up[0][1] = pcv[0] * pcv[2];
        up[0][2] = nsv[0] * ncv[1] * pcv[2] + psv[1] * psv[2];
    }
}
