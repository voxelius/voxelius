// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR3DI64_HH
#define SHARED_VECTOR3DI64_HH
#include <array>
#include <cstdint>
#include <utility>

// This is a crutched integer 3D vector used as a base
// type for world coordinates and their extension methods
class Vector3Di64 : public std::array<std::int64_t, 3> {
public:
    Vector3Di64(void) = default;
    constexpr Vector3Di64(std::int64_t filler);
    constexpr Vector3Di64(std::int64_t x, std::int64_t y, std::int64_t z);
    constexpr Vector3Di64(const Vector3Di64 &other);
    using std::array<std::int64_t, 3>::array;

public:
    constexpr Vector3Di64 operator+(const Vector3Di64 &vector) const;
    constexpr Vector3Di64 operator-(const Vector3Di64 &vector) const;
    constexpr Vector3Di64 operator+(const std::int64_t scalar) const;
    constexpr Vector3Di64 operator-(const std::int64_t scalar) const;
    constexpr Vector3Di64 operator*(const std::int64_t scalar) const;
    constexpr Vector3Di64 operator/(const std::int64_t scalar) const;

public:
    constexpr Vector3Di64 &operator+=(const Vector3Di64 &vector);
    constexpr Vector3Di64 &operator-=(const Vector3Di64 &vector);
    constexpr Vector3Di64 &operator+=(const std::int64_t scalar);
    constexpr Vector3Di64 &operator-=(const std::int64_t scalar);
    constexpr Vector3Di64 &operator*=(const std::int64_t scalar);
    constexpr Vector3Di64 &operator/=(const std::int64_t scalar);

public:
    constexpr static Vector3Di64 dir_north(void);
    constexpr static Vector3Di64 dir_south(void);
    constexpr static Vector3Di64 dir_east(void);
    constexpr static Vector3Di64 dir_west(void);
    constexpr static Vector3Di64 dir_down(void);
    constexpr static Vector3Di64 dir_up(void);
};

constexpr inline Vector3Di64::Vector3Di64(std::int64_t filler)
{
    this[0][0] = filler;
    this[0][1] = filler;
    this[0][2] = filler;
}

constexpr inline Vector3Di64::Vector3Di64(std::int64_t x, std::int64_t y, std::int64_t z)
{
    this[0][0] = x;
    this[0][1] = y;
    this[0][2] = z;
}

constexpr inline Vector3Di64::Vector3Di64(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline Vector3Di64 Vector3Di64::operator+(const Vector3Di64 &vector) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    return std::move(result);
}

constexpr inline Vector3Di64 Vector3Di64::operator-(const Vector3Di64 &vector) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    return std::move(result);
}

constexpr inline Vector3Di64 Vector3Di64::operator+(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

constexpr inline Vector3Di64 Vector3Di64::operator-(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

constexpr inline Vector3Di64 Vector3Di64::operator*(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

constexpr inline Vector3Di64 Vector3Di64::operator/(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

constexpr inline Vector3Di64 &Vector3Di64::operator+=(const Vector3Di64 &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    return this[0];
}

constexpr inline Vector3Di64 &Vector3Di64::operator-=(const Vector3Di64 &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    return this[0];
}


constexpr inline Vector3Di64 &Vector3Di64::operator+=(const std::int64_t scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

constexpr inline Vector3Di64 &Vector3Di64::operator-=(const std::int64_t scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}


constexpr inline Vector3Di64 &Vector3Di64::operator*=(const std::int64_t scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

constexpr inline Vector3Di64 &Vector3Di64::operator/=(const std::int64_t scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}

constexpr inline Vector3Di64 Vector3Di64::dir_north(void)
{
    return std::move(Vector3Di64(0, 0, 1));
}

constexpr inline Vector3Di64 Vector3Di64::dir_south(void)
{
    return std::move(Vector3Di64(0, 0, -1));
}

constexpr inline Vector3Di64 Vector3Di64::dir_east(void)
{
    return std::move(Vector3Di64(1, 0, 0));
}

constexpr inline Vector3Di64 Vector3Di64::dir_west(void)
{
    return std::move(Vector3Di64(-1, 0, 0));
}

constexpr inline Vector3Di64 Vector3Di64::dir_down(void)
{
    return std::move(Vector3Di64(0, -1, 0));
}

constexpr inline Vector3Di64 Vector3Di64::dir_up(void)
{
    return std::move(Vector3Di64(0, 1, 0));
}

#endif /* SHARED_VECTOR3DI64_HH */
