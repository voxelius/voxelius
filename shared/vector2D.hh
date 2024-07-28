// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR2D_HH
#define SHARED_VECTOR2D_HH
#include <array>
#include <cmath>

class Vector2D final : public std::array<float, 2> {
public:
    Vector2D(void) = default;
    constexpr Vector2D(float filler);
    constexpr Vector2D(float x, float y);
    constexpr Vector2D(const Vector2D &other);
    using std::array<float, 2>::array;

public:
    Vector2D operator+(const Vector2D &vector) const;
    Vector2D operator-(const Vector2D &vector) const;
    Vector2D operator+(const float scalar) const;
    Vector2D operator-(const float scalar) const;
    Vector2D operator*(const float scalar) const;
    Vector2D operator/(const float scalar) const;

public:
    Vector2D &operator+=(const Vector2D &vector);
    Vector2D &operator-=(const Vector2D &vector);
    Vector2D &operator+=(const float scalar);
    Vector2D &operator-=(const float scalar);
    Vector2D &operator*=(const float scalar);
    Vector2D &operator/=(const float scalar);

public:
    static float length(const Vector2D &vector);
    static float length2(const Vector2D &vector);
    static float normalize(Vector2D &vector);

public:
    static float angle(const Vector2D &avec, const Vector2D &bvec);
    static float dot_product(const Vector2D &avec, const Vector2D &bvec);

public:
    static Vector2D cos(const Vector2D &vector);
    static Vector2D sin(const Vector2D &vector);
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

#endif /* SHARED_VECTOR2D_HH */
