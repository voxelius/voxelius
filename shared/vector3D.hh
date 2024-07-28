// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR3D_HH
#define SHARED_VECTOR3D_HH
#include <array>
#include <cmath>

class Quaternion;
class Vector3D final : public std::array<float, 3> {
public:
    Vector3D(void) = default;
    constexpr Vector3D(float filler);
    constexpr Vector3D(float x, float y, float z);
    constexpr Vector3D(const Vector3D &other);
    using std::array<float, 3>::array;

public:
    Vector3D operator+(const Vector3D &vector) const;
    Vector3D operator-(const Vector3D &vector) const;
    Vector3D operator+(const float scalar) const;
    Vector3D operator-(const float scalar) const;
    Vector3D operator*(const float scalar) const;
    Vector3D operator/(const float scalar) const;

public:
    Vector3D &operator+=(const Vector3D &vector);
    Vector3D &operator-=(const Vector3D &vector);
    Vector3D &operator+=(const float scalar);
    Vector3D &operator-=(const float scalar);
    Vector3D &operator*=(const float scalar);
    Vector3D &operator/=(const float scalar);

public:
    static float length(const Vector3D &vector);
    static float length2(const Vector3D &vector);
    static float normalize(Vector3D &vector);

public:
    static float angle(const Vector3D &avec, const Vector3D &bvec);
    static float dot_product(const Vector3D &avec, const Vector3D &bvec);
    static Vector3D cross_product(const Vector3D &avec, const Vector3D &bvec);
};

constexpr inline Vector3D::Vector3D(float filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
    this[0][2] = filler;
}

constexpr inline Vector3D::Vector3D(float x, float y, float z)
{
    this[0][0] = x;
    this[0][1] = y;
    this[0][2] = z;
}

constexpr inline Vector3D::Vector3D(const Vector3D &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

#endif /* SHARED_VECTOR3D_HH */
