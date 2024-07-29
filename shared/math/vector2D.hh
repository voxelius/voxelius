// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR2D_HH
#define SHARED_VECTOR2D_HH
#include <array>
#include <cmath>
#include <utility>

class Vector2D final : public std::array<float, 2> {
public:
    Vector2D(void) = default;
    constexpr Vector2D(float filler);
    constexpr Vector2D(float x, float y);
    constexpr Vector2D(const Vector2D &other);
    using std::array<float, 2>::array;

public:
    constexpr Vector2D operator+(const Vector2D &vector) const;
    constexpr Vector2D operator-(const Vector2D &vector) const;
    constexpr Vector2D operator+(const float scalar) const;
    constexpr Vector2D operator-(const float scalar) const;
    constexpr Vector2D operator*(const float scalar) const;
    constexpr Vector2D operator/(const float scalar) const;

public:
    constexpr Vector2D &operator+=(const Vector2D &vector);
    constexpr Vector2D &operator-=(const Vector2D &vector);
    constexpr Vector2D &operator+=(const float scalar);
    constexpr Vector2D &operator-=(const float scalar);
    constexpr Vector2D &operator*=(const float scalar);
    constexpr Vector2D &operator/=(const float scalar);

public:
    static float length(const Vector2D &vector);
    static float length2(const Vector2D &vector);
    static float normalize(Vector2D &vector);

public:
    constexpr static float dot(const Vector2D &avec, const Vector2D &bvec);
};

constexpr inline Vector2D::Vector2D(float filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
}

constexpr inline Vector2D::Vector2D(float x, float y)
{
    this[0][0] = x;
    this[0][1] = y;
}

constexpr inline Vector2D::Vector2D(const Vector2D &other)
{
    this[0][0] = other[0];
    this[0][1] = other[2];
}

constexpr inline Vector2D Vector2D::operator+(const Vector2D &vector) const
{
    Vector2D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    return std::move(result);
}

constexpr inline Vector2D Vector2D::operator-(const Vector2D &vector) const
{
    Vector2D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    return std::move(result);
}

constexpr inline Vector2D Vector2D::operator+(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    return std::move(result);
}

constexpr inline Vector2D Vector2D::operator-(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    return std::move(result);
}

constexpr inline Vector2D Vector2D::operator*(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    return std::move(result);
}

constexpr inline Vector2D Vector2D::operator/(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    return std::move(result);
}

constexpr inline Vector2D &Vector2D::operator+=(const Vector2D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    return this[0];
}

constexpr inline Vector2D &Vector2D::operator-=(const Vector2D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    return this[0];
}


constexpr inline Vector2D &Vector2D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    return this[0];
}

constexpr inline Vector2D &Vector2D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    return this[0];
}


constexpr inline Vector2D &Vector2D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    return this[0];
}

constexpr inline Vector2D &Vector2D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    return this[0];
}

inline float Vector2D::length(const Vector2D &vector)
{
    return std::sqrt(Vector2D::dot(vector, vector));
}

inline float Vector2D::length2(const Vector2D &vector)
{
    return Vector2D::dot(vector, vector);
}

inline float Vector2D::normalize(Vector2D &vector)
{
    const float result = Vector2D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    return result;
}

constexpr inline float Vector2D::dot(const Vector2D &avec, const Vector2D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1];
}

#endif /* SHARED_VECTOR2D_HH */
