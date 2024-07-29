// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR3D_HH
#define SHARED_VECTOR3D_HH
#include <array>
#include <cmath>
#include <utility>

class Vector3D final : public std::array<float, 3> {
public:
    Vector3D(void) = default;
    constexpr Vector3D(float filler);
    constexpr Vector3D(float x, float y, float z);
    constexpr Vector3D(const Vector3D &other);
    using std::array<float, 3>::array;

public:
    constexpr Vector3D operator+(const Vector3D &vector) const;
    constexpr Vector3D operator-(const Vector3D &vector) const;
    constexpr Vector3D operator+(const float scalar) const;
    constexpr Vector3D operator-(const float scalar) const;
    constexpr Vector3D operator*(const float scalar) const;
    constexpr Vector3D operator/(const float scalar) const;

public:
    constexpr Vector3D &operator+=(const Vector3D &vector);
    constexpr Vector3D &operator-=(const Vector3D &vector);
    constexpr Vector3D &operator+=(const float scalar);
    constexpr Vector3D &operator-=(const float scalar);
    constexpr Vector3D &operator*=(const float scalar);
    constexpr Vector3D &operator/=(const float scalar);

public:
    static float length(const Vector3D &vector);
    static float length2(const Vector3D &vector);
    static float normalize(Vector3D &vector);

public:
    constexpr static float dot(const Vector3D &avec, const Vector3D &bvec);
    constexpr static Vector3D cross(const Vector3D &avec, const Vector3D &bvec);

public:
    constexpr static Vector3D dir_forward(void);
    constexpr static Vector3D dir_back(void);
    constexpr static Vector3D dir_left(void);
    constexpr static Vector3D dir_right(void);
    constexpr static Vector3D dir_down(void);
    constexpr static Vector3D dir_up(void);
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

constexpr inline Vector3D Vector3D::operator+(const Vector3D &vector) const
{
    Vector3D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    return std::move(result);
}

constexpr inline Vector3D Vector3D::operator-(const Vector3D &vector) const
{
    Vector3D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    return std::move(result);
}

constexpr inline Vector3D Vector3D::operator+(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

constexpr inline Vector3D Vector3D::operator-(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

constexpr inline Vector3D Vector3D::operator*(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

constexpr inline Vector3D Vector3D::operator/(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

constexpr inline Vector3D &Vector3D::operator+=(const Vector3D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    return this[0];
}

constexpr inline Vector3D &Vector3D::operator-=(const Vector3D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    return this[0];
}


constexpr inline Vector3D &Vector3D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

constexpr inline Vector3D &Vector3D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}


constexpr inline Vector3D &Vector3D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

constexpr inline Vector3D &Vector3D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}

inline float Vector3D::length(const Vector3D &vector)
{
    return std::sqrt(Vector3D::dot(vector, vector));
}

inline float Vector3D::length2(const Vector3D &vector)
{
    return Vector3D::dot(vector, vector);
}

inline float Vector3D::normalize(Vector3D &vector)
{
    const float result = Vector3D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    vector[2] /= result;
    return result;
}

constexpr inline float Vector3D::dot(const Vector3D &avec, const Vector3D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2];
}

constexpr inline Vector3D Vector3D::cross(const Vector3D &avec, const Vector3D &bvec)
{
    Vector3D result = {};
    result[0] = avec[1] * bvec[2] - avec[2] * bvec[1];
    result[1] = avec[2] * bvec[0] - avec[0] * bvec[2];
    result[2] = avec[0] * bvec[1] - avec[1] * bvec[0];
    return std::move(result);
}

constexpr inline Vector3D Vector3D::dir_forward(void)
{
    return std::move(Vector3D(0.0f, 0.0f, 1.0f));
}

constexpr inline Vector3D Vector3D::dir_back(void)
{
    return std::move(Vector3D(0.0f, 0.0f, -1.0f));
}

constexpr inline Vector3D Vector3D::dir_left(void)
{
    return std::move(Vector3D(-1.0f, 0.0f, 0.0f));
}

constexpr inline Vector3D Vector3D::dir_right(void)
{
    return std::move(Vector3D(1.0f, 0.0f, 0.0f));
}

constexpr inline Vector3D Vector3D::dir_down(void)
{
    return std::move(Vector3D(0.0f, -1.0f, 0.0f));
}

constexpr inline Vector3D Vector3D::dir_up(void)
{
    return std::move(Vector3D(0.0f, 1.0f, 0.0f));
}

#endif /* SHARED_VECTOR3D_HH */
