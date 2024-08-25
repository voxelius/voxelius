// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec3base.hh>

class Vec3f final : public Vec3base<float> {
public:
    template<typename T>
    constexpr Vec3f(const Vec3base<T> &other);
    using Vec3base<float>::Vec3base;

public:
    constexpr static Vec3f cross(const Vec3f &avec, const Vec3f &bvec);
    constexpr static float dot(const Vec3f &avec, const Vec3f &bvec);

public:
    static float length(const Vec3f &vector);
    static float length2(const Vec3f &vector);
    static float normalize(Vec3f &vector);
};

template<typename T>
constexpr inline Vec3f::Vec3f(const Vec3base<T> &other)
{
    this[0][0] = static_cast<float>(other[0]);
    this[0][1] = static_cast<float>(other[1]);
    this[0][2] = static_cast<float>(other[2]);
}

constexpr inline Vec3f Vec3f::cross(const Vec3f &avec, const Vec3f &bvec)
{
    Vec3f result = {};
    result[0] = avec[1] * bvec[2] - avec[2] * bvec[1];
    result[1] = avec[2] * bvec[0] - avec[0] * bvec[2];
    result[2] = avec[0] * bvec[1] - avec[1] * bvec[0];
    return std::move(result);
}

constexpr inline float Vec3f::dot(const Vec3f &avec, const Vec3f &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2];
}
