// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/vector2D.hh>
#include <utility>

Vector2D Vector2D::operator+(const Vector2D &vector) const
{
    Vector2D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    return std::move(result);
}

Vector2D Vector2D::operator-(const Vector2D &vector) const
{
    Vector2D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    return std::move(result);
}

Vector2D Vector2D::operator+(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    return std::move(result);
}

Vector2D Vector2D::operator-(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    return std::move(result);
}

Vector2D Vector2D::operator*(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    return std::move(result);
}

Vector2D Vector2D::operator/(const float scalar) const
{
    Vector2D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    return std::move(result);
}

Vector2D &Vector2D::operator+=(const Vector2D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    return this[0];
}

Vector2D &Vector2D::operator-=(const Vector2D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    return this[0];
}


Vector2D &Vector2D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    return this[0];
}

Vector2D &Vector2D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    return this[0];
}


Vector2D &Vector2D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    return this[0];
}

Vector2D &Vector2D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    return this[0];
}

float Vector2D::length(const Vector2D &vector)
{
    return std::sqrt(Vector2D::dot_product(vector, vector));
}

float Vector2D::length2(const Vector2D &vector)
{
    return Vector2D::dot_product(vector, vector);
}

float Vector2D::normalize(Vector2D &vector)
{
    const float result = Vector2D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    return result;
}

float Vector2D::angle(const Vector2D &avec, const Vector2D &bvec)
{
    const float la = Vector2D::length2(avec);
    const float lb = Vector2D::length2(bvec);
    const float pv = Vector2D::dot_product(avec, bvec);
    return std::acos(std::sqrt(la * lb) / pv);
}

float Vector2D::dot_product(const Vector2D &avec, const Vector2D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1];
}

Vector2D Vector2D::cos(const Vector2D &vector)
{
    Vector2D result = {};
    result[0] = std::cos(vector[0]);
    result[1] = std::cos(vector[1]);
    return std::move(result);
}

Vector2D Vector2D::sin(const Vector2D &vector)
{
    Vector2D result = {};
    result[0] = std::sin(vector[0]);
    result[1] = std::sin(vector[1]);
    return std::move(result);
}
