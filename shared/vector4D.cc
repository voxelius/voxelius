// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/vector4D.hh>
#include <utility>

Vector4D Vector4D::operator+(const Vector4D &vector) const
{
    Vector4D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    result[3] = this[0][3] + vector[3];
    return std::move(result);
}

Vector4D Vector4D::operator-(const Vector4D &vector) const
{
    Vector4D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    result[3] = this[0][3] - vector[3];
    return std::move(result);
}

Vector4D Vector4D::operator+(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    result[3] = this[0][3] + scalar;
    return std::move(result);
}

Vector4D Vector4D::operator-(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    result[3] = this[0][3] - scalar;
    return std::move(result);
}

Vector4D Vector4D::operator*(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    result[3] = this[0][3] * scalar;
    return std::move(result);
}

Vector4D Vector4D::operator/(const float scalar) const
{
    Vector4D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    result[3] = this[0][3] / scalar;
    return std::move(result);
}

Vector4D &Vector4D::operator+=(const Vector4D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    this[0][3] += vector[3];
    return this[0];
}

Vector4D &Vector4D::operator-=(const Vector4D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    this[0][3] -= vector[3];
    return this[0];
}


Vector4D &Vector4D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    this[0][3] += scalar;
    return this[0];
}

Vector4D &Vector4D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    this[0][3] -= scalar;
    return this[0];
}


Vector4D &Vector4D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    this[0][3] *= scalar;
    return this[0];
}

Vector4D &Vector4D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    this[0][3] /= scalar;
    return this[0];
}

float Vector4D::length(const Vector4D &vector)
{
    return std::sqrt(Vector4D::dot_product(vector, vector));
}

float Vector4D::length2(const Vector4D &vector)
{
    return Vector4D::dot_product(vector, vector);
}

float Vector4D::normalize(Vector4D &vector)
{
    const float result = Vector4D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    vector[2] /= result;
    vector[3] /= result;
    return result;
}

float Vector4D::dot_product(const Vector4D &avec, const Vector4D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2] + avec[3] * bvec[3];
}

Vector4D Vector4D::cos(const Vector4D &vector)
{
    Vector4D result = {};
    result[0] = std::cos(vector[0]);
    result[1] = std::cos(vector[1]);
    result[2] = std::cos(vector[2]);
    result[3] = std::cos(vector[3]);
    return std::move(result);
}

Vector4D Vector4D::sin(const Vector4D &vector)
{
    Vector4D result = {};
    result[0] = std::sin(vector[0]);
    result[1] = std::sin(vector[1]);
    result[2] = std::sin(vector[2]);
    result[3] = std::sin(vector[3]);
    return std::move(result);
}
