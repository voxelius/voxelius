// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VECTOR3DI64_HH
#define SHARED_VECTOR3DI64_HH
#include <array>
#include <cstdint>

// This is a crutched vector used as a base
// type for world coordinates and their extension methods
class Vector3Di64 : public std::array<std::int64_t, 3> {
public:
    Vector3Di64(void) = default;
    constexpr Vector3Di64(std::int64_t filler);
    constexpr Vector3Di64(std::int64_t x, std::int64_t y, std::int64_t z);
    constexpr Vector3Di64(const Vector3Di64 &other);
    using std::array<std::int64_t, 3>::array;

public:
    Vector3Di64 operator+(const Vector3Di64 &vector) const;
    Vector3Di64 operator-(const Vector3Di64 &vector) const;
    Vector3Di64 operator+(const std::int64_t scalar) const;
    Vector3Di64 operator-(const std::int64_t scalar) const;
    Vector3Di64 operator*(const std::int64_t scalar) const;
    Vector3Di64 operator/(const std::int64_t scalar) const;

public:
    Vector3Di64 &operator+=(const Vector3Di64 &vector);
    Vector3Di64 &operator-=(const Vector3Di64 &vector);
    Vector3Di64 &operator+=(const std::int64_t scalar);
    Vector3Di64 &operator-=(const std::int64_t scalar);
    Vector3Di64 &operator*=(const std::int64_t scalar);
    Vector3Di64 &operator/=(const std::int64_t scalar);
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

#endif /* SHARED_VECTOR3DI64_HH */
