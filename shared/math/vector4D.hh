// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR4D_HH
#define SHARED_VECTOR4D_HH
#include <array>
#include <cmath>
#include <utility>

class Vector4D final : public std::array<float, 4> {
public:
    Vector4D(void) = default;
    constexpr Vector4D(float filler);
    constexpr Vector4D(float x, float y, float z, float w);
    constexpr Vector4D(const Vector4D &other);
    using std::array<float, 4>::array;

public:
    constexpr Vector4D operator+(const Vector4D &vector) const;
    constexpr Vector4D operator-(const Vector4D &vector) const;
    constexpr Vector4D operator+(const float scalar) const;
    constexpr Vector4D operator-(const float scalar) const;
    constexpr Vector4D operator*(const float scalar) const;
    constexpr Vector4D operator/(const float scalar) const;

public:
    constexpr Vector4D &operator+=(const Vector4D &vector);
    constexpr Vector4D &operator-=(const Vector4D &vector);
    constexpr Vector4D &operator+=(const float scalar);
    constexpr Vector4D &operator-=(const float scalar);
    constexpr Vector4D &operator*=(const float scalar);
    constexpr Vector4D &operator/=(const float scalar);

public:
    static float length(const Vector4D &vector);
    static float length2(const Vector4D &vector);
    static float normalize(Vector4D &vector);

public:
    constexpr static float dot(const Vector4D &avec, const Vector4D &bvec);

public:
    constexpr static Vector4D black(void);
    constexpr static Vector4D blue(void);
    constexpr static Vector4D brown(void);
    constexpr static Vector4D cyan(void);
    constexpr static Vector4D dark_blue(void);
    constexpr static Vector4D dark_cyan(void);
    constexpr static Vector4D dark_green(void);
    constexpr static Vector4D dark_magenta(void);
    constexpr static Vector4D dark_red(void);
    constexpr static Vector4D gray(void);
    constexpr static Vector4D green(void);
    constexpr static Vector4D light_gray(void);
    constexpr static Vector4D magenta(void);
    constexpr static Vector4D red(void);
    constexpr static Vector4D transparent(void);
    constexpr static Vector4D white(void);
    constexpr static Vector4D yellow(void);
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

constexpr inline Vector4D Vector4D::operator+(const Vector4D &vector) const
{
    Vector4D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    result[3] = this[0][3] + vector[3];
    return std::move(result);
}

constexpr inline Vector4D Vector4D::operator-(const Vector4D &vector) const
{
    Vector4D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    result[3] = this[0][3] - vector[3];
    return std::move(result);
}

constexpr inline Vector4D Vector4D::operator+(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    result[3] = this[0][3] + scalar;
    return std::move(result);
}

constexpr inline Vector4D Vector4D::operator-(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    result[3] = this[0][3] - scalar;
    return std::move(result);
}

constexpr inline Vector4D Vector4D::operator*(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    result[3] = this[0][3] * scalar;
    return std::move(result);
}

constexpr inline Vector4D Vector4D::operator/(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    result[3] = this[0][3] / scalar;
    return std::move(result);
}

constexpr inline Vector4D &Vector4D::operator+=(const Vector4D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    this[0][3] += vector[3];
    return this[0];
}

constexpr inline Vector4D &Vector4D::operator-=(const Vector4D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    this[0][3] -= vector[3];
    return this[0];
}


constexpr inline Vector4D &Vector4D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    this[0][3] += scalar;
    return this[0];
}

constexpr inline Vector4D &Vector4D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    this[0][3] -= scalar;
    return this[0];
}


constexpr inline Vector4D &Vector4D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    this[0][3] *= scalar;
    return this[0];
}

constexpr inline Vector4D &Vector4D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    this[0][3] /= scalar;
    return this[0];
}

inline float Vector4D::length(const Vector4D &vector)
{
    return std::sqrt(Vector4D::dot(vector, vector));
}

inline float Vector4D::length2(const Vector4D &vector)
{
    return Vector4D::dot(vector, vector);
}

inline float Vector4D::normalize(Vector4D &vector)
{
    const float result = Vector4D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    vector[2] /= result;
    vector[3] /= result;
    return result;
}

constexpr inline float Vector4D::dot(const Vector4D &avec, const Vector4D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2] + avec[3] * bvec[3];
}

constexpr inline Vector4D Vector4D::black(void)
{
    return std::move(Vector4D(0.00f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::blue(void)
{
    return std::move(Vector4D(0.00f, 0.00f, 1.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::brown(void)
{
    return std::move(Vector4D(0.50f, 0.50f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::cyan(void)
{
    return std::move(Vector4D(0.00f, 1.00f, 1.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::dark_blue(void)
{
    return std::move(Vector4D(0.00f, 0.00f, 0.50f, 1.00f));
}

constexpr inline Vector4D Vector4D::dark_cyan(void)
{
    return std::move(Vector4D(0.00f, 0.50f, 0.50f, 1.00f));
}

constexpr inline Vector4D Vector4D::dark_green(void)
{
    return std::move(Vector4D(0.00f, 0.50f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::dark_magenta(void)
{
    return std::move(Vector4D(0.50f, 0.00f, 0.50f, 1.00f));
}

constexpr inline Vector4D Vector4D::dark_red(void)
{
    return std::move(Vector4D(0.50f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::gray(void)
{
    return std::move(Vector4D(0.50f, 0.50f, 0.50f, 1.00f));
}

constexpr inline Vector4D Vector4D::green(void)
{
    return std::move(Vector4D(0.00f, 1.00f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::light_gray(void)
{
    return std::move(Vector4D(0.80f, 0.80f, 0.80f, 1.00f));
}

constexpr inline Vector4D Vector4D::magenta(void)
{
    return std::move(Vector4D(1.00f, 0.00f, 1.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::red(void)
{
    return std::move(Vector4D(1.00f, 0.00f, 0.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::transparent(void)
{
    return std::move(Vector4D(0.00f, 0.00f, 0.00f, 0.00f));
}

constexpr inline Vector4D Vector4D::white(void)
{
    return std::move(Vector4D(1.00f, 1.00f, 1.00f, 1.00f));
}

constexpr inline Vector4D Vector4D::yellow(void)
{
    return std::move(Vector4D(1.00f, 1.00f, 0.00f, 1.00f));
}

#endif /* SHARED_VECTOR4D_HH */
