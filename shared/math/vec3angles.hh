// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_VEC3ANGLES_HH
#define SHARED_MATH_VEC3ANGLES_HH
#include <shared/math/constexpr.hh>
#include <shared/math/vec3f.hh>

class Vec3angles final : public Vec3base<float> {
public:
    template<typename T>
    constexpr Vec3angles(const Vec3base<T> &other);
    using Vec3base<float>::Vec3base;

public:
    constexpr static Vec3angles degrees(const Vec3angles &angles);
    constexpr static Vec3angles radians(const Vec3angles &angles);

public:
    static Vec3angles wrap_180(const Vec3angles &angles);
    static Vec3angles wrap_360(const Vec3angles &angles);

public:
    static Vec3f cos(const Vec3angles &angles);
    static Vec3f sin(const Vec3angles &angles);

public:
    static void vectors(const Vec3angles &angles, Vec3f &forward);
    static void vectors(const Vec3angles &angles, Vec3f *forward, Vec3f *right, Vec3f *up);
};

template<typename T>
constexpr inline Vec3angles::Vec3angles(const Vec3base<T> &other)
{
    this[0][0] = static_cast<float>(other[0]);
    this[0][1] = static_cast<float>(other[1]);
    this[0][2] = static_cast<float>(other[2]);
}

constexpr inline Vec3angles Vec3angles::degrees(const Vec3angles &angles)
{
    Vec3angles result = {};
    result[0] = cxpr::degrees(angles[0]);
    result[1] = cxpr::degrees(angles[1]);
    result[2] = cxpr::degrees(angles[2]);
    return std::move(result);
}

constexpr inline Vec3angles Vec3angles::radians(const Vec3angles &angles)
{
    Vec3angles result = {};
    result[0] = cxpr::radians(angles[0]);
    result[1] = cxpr::radians(angles[1]);
    result[2] = cxpr::radians(angles[2]);
    return std::move(result);
}

#endif /* SHARED_MATH_VEC3ANGLES_HH */
