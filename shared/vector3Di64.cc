// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/vector3Di64.hh>
#include <utility>

Vector3Di64 Vector3Di64::operator+(const Vector3Di64 &vector) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    return std::move(result);
}

Vector3Di64 Vector3Di64::operator-(const Vector3Di64 &vector) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    return std::move(result);
}

Vector3Di64 Vector3Di64::operator+(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

Vector3Di64 Vector3Di64::operator-(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

Vector3Di64 Vector3Di64::operator*(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

Vector3Di64 Vector3Di64::operator/(const std::int64_t scalar) const
{
    Vector3Di64 result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

Vector3Di64 &Vector3Di64::operator+=(const Vector3Di64 &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    return this[0];
}

Vector3Di64 &Vector3Di64::operator-=(const Vector3Di64 &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    return this[0];
}


Vector3Di64 &Vector3Di64::operator+=(const std::int64_t scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

Vector3Di64 &Vector3Di64::operator-=(const std::int64_t scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}


Vector3Di64 &Vector3Di64::operator*=(const std::int64_t scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

Vector3Di64 &Vector3Di64::operator/=(const std::int64_t scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}
