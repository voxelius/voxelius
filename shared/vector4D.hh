// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR4D_HH
#define SHARED_VECTOR4D_HH
#include <array>
#include <cmath>

class Vector4D final : public std::array<float, 4> {
public:
    Vector4D(void) = default;
    constexpr Vector4D(float filler);
    constexpr Vector4D(float x, float y, float z, float w);
    constexpr Vector4D(const Vector4D &other);
    using std::array<float, 4>::array;

public:
    Vector4D operator+(const Vector4D &vector) const;
    Vector4D operator-(const Vector4D &vector) const;
    Vector4D operator+(const float scalar) const;
    Vector4D operator-(const float scalar) const;
    Vector4D operator*(const float scalar) const;
    Vector4D operator/(const float scalar) const;

public:
    Vector4D &operator+=(const Vector4D &vector);
    Vector4D &operator-=(const Vector4D &vector);
    Vector4D &operator+=(const float scalar);
    Vector4D &operator-=(const float scalar);
    Vector4D &operator*=(const float scalar);
    Vector4D &operator/=(const float scalar);

public:
    static float length(const Vector4D &vector);
    static float length2(const Vector4D &vector);
    static float normalize(Vector4D &vector);

public:
    static float dot_product(const Vector4D &avec, const Vector4D &bvec);

public:
    static Vector4D cos(const Vector4D &vector);
    static Vector4D sin(const Vector4D &vector);
};

constexpr inline Vector4D::Vector4D(float filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
    this[0][2] = filler;
    this[0][3] = filler;
}

constexpr inline Vector4D::Vector4D(float x, float y, float z, float w)
{
    this[0][0] = x;
    this[0][1] = y;
    this[0][2] = z;
    this[0][3] = w;
}

constexpr inline Vector4D::Vector4D(const Vector4D &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
    this[0][3] = other[3];
}

#endif /* SHARED_VECTOR4D_HH */
