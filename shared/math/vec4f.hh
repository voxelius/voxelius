// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_VEC4F_HH
#define SHARED_MATH_VEC4F_HH
#include <shared/math/vec4base.hh>

class Vec4f final : public Vec4base<float> {
public:
    template<typename T>
    constexpr Vec4f(const Vec4base<T> &other);
    using Vec4base<float>::Vec4base;

public:
    constexpr static float dot(const Vec4f &avec, const Vec4f &bvec);

public:
    static float length(const Vec4f &vector);
    static float length2(const Vec4f &vector);
    static float normalize(Vec4f &vector);

public:
    constexpr static Vec4f black(void);
    constexpr static Vec4f blue(void);
    constexpr static Vec4f brown(void);
    constexpr static Vec4f cyan(void);
    constexpr static Vec4f dark_blue(void);
    constexpr static Vec4f dark_cyan(void);
    constexpr static Vec4f dark_green(void);
    constexpr static Vec4f dark_magenta(void);
    constexpr static Vec4f dark_red(void);
    constexpr static Vec4f gray(void);
    constexpr static Vec4f green(void);
    constexpr static Vec4f light_gray(void);
    constexpr static Vec4f magenta(void);
    constexpr static Vec4f red(void);
    constexpr static Vec4f transparent(void);
    constexpr static Vec4f white(void);
    constexpr static Vec4f yellow(void);
};

template<typename T>
constexpr inline Vec4f::Vec4f(const Vec4base<T> &other)
{
    this[0][0] = static_cast<float>(other[0]);
    this[0][1] = static_cast<float>(other[1]);
    this[0][2] = static_cast<float>(other[2]);
    this[0][3] = static_cast<float>(other[3]);
}

constexpr inline float Vec4f::dot(const Vec4f &avec, const Vec4f &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2] + avec[3] * bvec[3];
}

constexpr inline Vec4f Vec4f::black(void)
{
    return std::move(Vec4f(0.00f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::blue(void)
{
    return std::move(Vec4f(0.00f, 0.00f, 1.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::brown(void)
{
    return std::move(Vec4f(0.50f, 0.50f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::cyan(void)
{
    return std::move(Vec4f(0.00f, 1.00f, 1.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::dark_blue(void)
{
    return std::move(Vec4f(0.00f, 0.00f, 0.50f, 1.00f));
}

constexpr inline Vec4f Vec4f::dark_cyan(void)
{
    return std::move(Vec4f(0.00f, 0.50f, 0.50f, 1.00f));
}

constexpr inline Vec4f Vec4f::dark_green(void)
{
    return std::move(Vec4f(0.00f, 0.50f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::dark_magenta(void)
{
    return std::move(Vec4f(0.50f, 0.00f, 0.50f, 1.00f));
}

constexpr inline Vec4f Vec4f::dark_red(void)
{
    return std::move(Vec4f(0.50f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::gray(void)
{
    return std::move(Vec4f(0.50f, 0.50f, 0.50f, 1.00f));
}

constexpr inline Vec4f Vec4f::green(void)
{
    return std::move(Vec4f(0.00f, 1.00f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::light_gray(void)
{
    return std::move(Vec4f(0.80f, 0.80f, 0.80f, 1.00f));
}

constexpr inline Vec4f Vec4f::magenta(void)
{
    return std::move(Vec4f(1.00f, 0.00f, 1.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::red(void)
{
    return std::move(Vec4f(1.00f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::transparent(void)
{
    return std::move(Vec4f(0.00f, 0.00f, 0.00f, 0.00f));
}

constexpr inline Vec4f Vec4f::white(void)
{
    return std::move(Vec4f(1.00f, 1.00f, 1.00f, 1.00f));
}

constexpr inline Vec4f Vec4f::yellow(void)
{
    return std::move(Vec4f(1.00f, 1.00f, 0.00f, 1.00f));
}

#endif /* SHARED_MATH_VEC4F_HH */
