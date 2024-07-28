// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/const.hh>
#include <shared/vector3D.hh>
#include <utility>

Vector3D Vector3D::operator+(const Vector3D &vector) const
{
    Vector3D result = {};
    result[0] = this[0][0] + vector[0];
    result[1] = this[0][1] + vector[1];
    result[2] = this[0][2] + vector[2];
    return std::move(result);
}

Vector3D Vector3D::operator-(const Vector3D &vector) const
{
    Vector3D result = {};
    result[0] = this[0][0] - vector[0];
    result[1] = this[0][1] - vector[1];
    result[2] = this[0][2] - vector[2];
    return std::move(result);
}

Vector3D Vector3D::operator+(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

Vector3D Vector3D::operator-(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

Vector3D Vector3D::operator*(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

Vector3D Vector3D::operator/(const float scalar) const
{
    Vector3D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

Vector3D &Vector3D::operator+=(const Vector3D &vector)
{
    this[0][0] += vector[0];
    this[0][1] += vector[1];
    this[0][2] += vector[2];
    return this[0];
}

Vector3D &Vector3D::operator-=(const Vector3D &vector)
{
    this[0][0] -= vector[0];
    this[0][1] -= vector[1];
    this[0][2] -= vector[2];
    return this[0];
}


Vector3D &Vector3D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

Vector3D &Vector3D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}


Vector3D &Vector3D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

Vector3D &Vector3D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}

float Vector3D::length(const Vector3D &vector)
{
    return std::sqrt(Vector3D::dot_product(vector, vector));
}

float Vector3D::length2(const Vector3D &vector)
{
    return Vector3D::dot_product(vector, vector);
}

float Vector3D::normalize(Vector3D &vector)
{
    const float result = Vector3D::length(vector);
    vector[0] /= result;
    vector[1] /= result;
    vector[2] /= result;
    return result;
}

float Vector3D::angle(const Vector3D &avec, const Vector3D &bvec)
{
    const float la = Vector3D::length2(avec);
    const float lb = Vector3D::length2(bvec);
    const float pv = Vector3D::dot_product(avec, bvec);
    return std::acos(std::sqrt(la * lb) / pv);
}

float Vector3D::dot_product(const Vector3D &avec, const Vector3D &bvec)
{
    return avec[0] * bvec[0] + avec[1] * bvec[1] + avec[2] * bvec[2];
}

Vector3D Vector3D::cross_product(const Vector3D &avec, const Vector3D &bvec)
{
    Vector3D result = {};
    result[0] = avec[1] * bvec[2] - avec[2] * bvec[1];
    result[1] = avec[2] * bvec[0] - avec[0] * bvec[2];
    result[2] = avec[0] * bvec[1] - avec[1] * bvec[0];
    return std::move(result);
}
