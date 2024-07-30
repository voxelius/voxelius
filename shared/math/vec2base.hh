// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_VEC2BASE_HH
#define SHARED_MATH_VEC2BASE_HH
#include <array>
#include <cmath>
#include <type_traits>
#include <utility>

template<typename T>
class Vec2base : public std::array<T, 2> {
    static_assert(std::is_arithmetic_v<T>);

public:
    Vec2base(void) = default;
    constexpr Vec2base(T filler);
    constexpr Vec2base(T x, T y);
    constexpr Vec2base(const Vec2base<T> &other);
    using std::array<T, 2>::array;

public:
    constexpr inline float get_x(void) const { return this[0][0]; }
    constexpr inline float get_y(void) const { return this[0][1]; }

public:
    constexpr inline void set_x(float value) { this[0][0] = value; }
    constexpr inline void set_y(float value) { this[0][1] = value; }

public:
    constexpr Vec2base<T> operator+(const Vec2base<T> &vector) const;
    constexpr Vec2base<T> operator-(const Vec2base<T> &vector) const;
    constexpr Vec2base<T> operator+(const T scalar) const;
    constexpr Vec2base<T> operator-(const T scalar) const;
    constexpr Vec2base<T> operator*(const T scalar) const;
    constexpr Vec2base<T> operator/(const T scalar) const;
    
public:
    constexpr Vec2base<T> &operator+=(const Vec2base<T> &vector);
    constexpr Vec2base<T> &operator-=(const Vec2base<T> &vector);
    constexpr Vec2base<T> &operator+=(const T scalar);
    constexpr Vec2base<T> &operator-=(const T scalar);
    constexpr Vec2base<T> &operator*=(const T scalar);
    constexpr Vec2base<T> &operator/=(const T scalar);
};

template<typename T>
constexpr inline Vec2base<T>::Vec2base(T filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
}
template<typename T>
constexpr inline Vec2base<T>::Vec2base(T x, T y)
{
    this[0][0] = x;
    this[0][1] = y;
}
template<typename T>
constexpr inline Vec2base<T>::Vec2base(const Vec2base<T> &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator+(const Vec2base<T> &vector) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator-(const Vec2base<T> &vector) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator+(const T scalar) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator-(const T scalar) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator*(const T scalar) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> Vec2base<T>::operator/(const T scalar) const
{
    Vec2base<T> result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator+=(const Vec2base<T> &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    return this[0];
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator-=(const Vec2base<T> &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    return this[0];
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator+=(const T scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator-=(const T scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator*=(const T scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec2base<T> &Vec2base<T>::operator/=(const T scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    return this[0];
}

#endif /* SHARED_MATH_VEC2BASE_HH */
