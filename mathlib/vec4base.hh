// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/common.hh>

template<typename T>
class Vec4base : public std::array<T, 4> {
    static_assert(std::is_arithmetic_v<T>);

public:
    Vec4base(void) = default;
    constexpr Vec4base(T filler);
    constexpr Vec4base(T x, T y, T z, T w);
    constexpr Vec4base(const Vec4base<T> &other);
    using std::array<T, 4>::array;

public:
    constexpr inline float get_x(void) const { return this[0][0]; }
    constexpr inline float get_y(void) const { return this[0][1]; }
    constexpr inline float get_z(void) const { return this[0][2]; }
    constexpr inline float get_w(void) const { return this[0][3]; }

public:
    constexpr inline void set_x(float value) { this[0][0] = value; }
    constexpr inline void set_y(float value) { this[0][1] = value; }
    constexpr inline void set_z(float value) { this[0][2] = value; }
    constexpr inline void set_w(float value) { this[0][3] = value; }

public:
    constexpr Vec4base<T> operator+(const Vec4base<T> &vector) const;
    constexpr Vec4base<T> operator-(const Vec4base<T> &vector) const;
    constexpr Vec4base<T> operator+(const T scalar) const;
    constexpr Vec4base<T> operator-(const T scalar) const;
    constexpr Vec4base<T> operator*(const T scalar) const;
    constexpr Vec4base<T> operator/(const T scalar) const;
    
public:
    constexpr Vec4base<T> &operator+=(const Vec4base<T> &vector);
    constexpr Vec4base<T> &operator-=(const Vec4base<T> &vector);
    constexpr Vec4base<T> &operator+=(const T scalar);
    constexpr Vec4base<T> &operator-=(const T scalar);
    constexpr Vec4base<T> &operator*=(const T scalar);
    constexpr Vec4base<T> &operator/=(const T scalar);
};

template<typename T>
constexpr inline Vec4base<T>::Vec4base(T filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
    this[0][2] = filler;
    this[0][3] = filler;
}
template<typename T>
constexpr inline Vec4base<T>::Vec4base(T x, T y, T z, T w)
{
    this[0][0] = x;
    this[0][1] = y;
    this[0][2] = z;
    this[0][3] = w;
}
template<typename T>
constexpr inline Vec4base<T>::Vec4base(const Vec4base<T> &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
    this[0][3] = other[3];
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator+(const Vec4base<T> &vector) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    result[3] = this[0][3] + vector[3];
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator-(const Vec4base<T> &vector) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    result[3] = this[0][3] - vector[3];
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator+(const T scalar) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    result[3] = this[0][3] + scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator-(const T scalar) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    result[3] = this[0][3] - scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator*(const T scalar) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    result[3] = this[0][3] * scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Vec4base<T>::operator/(const T scalar) const
{
    Vec4base<T> result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    result[3] = this[0][3] / scalar;
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator+=(const Vec4base<T> &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    this[0][3] += vector[3];
    return this[0];
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator-=(const Vec4base<T> &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    this[0][3] -= vector[3];
    return this[0];
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator+=(const T scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    this[0][3] += scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator-=(const T scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    this[0][3] -= scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator*=(const T scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    this[0][3] *= scalar;
    return this[0];
}

template<typename T>
constexpr inline Vec4base<T> &Vec4base<T>::operator/=(const T scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    this[0][3] /= scalar;
    return this[0];
}
