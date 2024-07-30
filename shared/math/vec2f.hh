// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_VEC2F_HH
#define SHARED_MATH_VEC2F_HH
#include <shared/math/vec2base.hh>

class Vec2f final : public Vec2base<float> {
public:
    template<typename T>
    constexpr Vec2f(const Vec2base<T> &other);
    using Vec2base<float>::Vec2base;

public:
    constexpr static float dot(const Vec2f &avec, const Vec2f &bvec);

public:
    static float length(const Vec2f &vector);
    static float length2(const Vec2f &vector);
    static float normalize(Vec2f &vector);
};

template<typename T>
constexpr inline Vec2f::Vec2f(const Vec2base<T> &other)
{
    this[0][0] = static_cast<float>(other[0]);
    this[0][1] = static_cast<float>(other[1]);
}

constexpr inline float Vec2f::dot(const Vec2f &avec, const Vec2f &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1];
}

#endif /* SHARED_MATH_VEC2F_HH */
