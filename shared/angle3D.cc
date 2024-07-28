// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/angle3D.hh>
#include <shared/const.hh>
#include <utility>

Angle3D Angle3D::operator+(const Angle3D &angles) const
{
    Angle3D result = {};
    result[0] = this[0][0] + angles[0];
    result[1] = this[0][1] + angles[1];
    result[2] = this[0][2] + angles[2];
    return std::move(result);
}

Angle3D Angle3D::operator-(const Angle3D &angles) const
{
    Angle3D result = {};
    result[0] = this[0][0] - angles[0];
    result[1] = this[0][1] - angles[1];
    result[2] = this[0][2] - angles[2];
    return std::move(result);
}

Angle3D Angle3D::operator+(const float scalar) const
{
    Angle3D result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

Angle3D Angle3D::operator-(const float scalar) const
{
    Angle3D result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

Angle3D Angle3D::operator*(const float scalar) const
{
    Angle3D result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

Angle3D Angle3D::operator/(const float scalar) const
{
    Angle3D result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

Angle3D &Angle3D::operator+=(const Angle3D &angles)
{
    this[0][0] += angles[0];
    this[0][1] += angles[1];
    this[0][2] += angles[2];
    return this[0];
}

Angle3D &Angle3D::operator-=(const Angle3D &angles)
{
    this[0][0] -= angles[0];
    this[0][1] -= angles[1];
    this[0][2] -= angles[2];
    return this[0];
}

Angle3D &Angle3D::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

Angle3D &Angle3D::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}

Angle3D &Angle3D::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

Angle3D &Angle3D::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}

Vector3D Angle3D::cos(const Angle3D &angles)
{
    Vector3D result = {};
    result[0] = std::cos(angles[0]);
    result[1] = std::cos(angles[1]);
    result[2] = std::cos(angles[2]);
    return std::move(result);
}

Vector3D Angle3D::sin(const Angle3D &angles)
{
    Vector3D result = {};
    result[0] = std::sin(angles[0]);
    result[1] = std::sin(angles[1]);
    result[2] = std::sin(angles[2]);
    return std::move(result);
}

Angle3D Angle3D::wrap_180(const Angle3D &angles)
{
    Angle3D result = {};
    result[0] = std::fmod(angles[0] + ANGLE_180D, ANGLE_360D);
    result[1] = std::fmod(angles[1] + ANGLE_180D, ANGLE_360D);
    result[2] = std::fmod(angles[2] + ANGLE_180D, ANGLE_360D);
    result[0] = ((result[0] < 0.0f) ? (result[0] + ANGLE_360D) : result[0]) - ANGLE_180D;
    result[1] = ((result[1] < 0.0f) ? (result[1] + ANGLE_360D) : result[1]) - ANGLE_180D;
    result[2] = ((result[2] < 0.0f) ? (result[2] + ANGLE_360D) : result[2]) - ANGLE_180D;
    return std::move(result);
}

Angle3D Angle3D::wrap_360(const Angle3D &angles)
{
    Angle3D result = {};
    result[0] = std::fmod(std::fmod(angles[0], ANGLE_360D) + ANGLE_360D, ANGLE_360D);
    result[1] = std::fmod(std::fmod(angles[1], ANGLE_360D) + ANGLE_360D, ANGLE_360D);
    result[2] = std::fmod(std::fmod(angles[2], ANGLE_360D) + ANGLE_360D, ANGLE_360D);
    return std::move(result);
}

void Angle3D::vectors(const Angle3D &angles, Vector3D *forward, Vector3D *right, Vector3D *up)
{
    if(!forward && !right && !up) {
        // There's no point in figuring out
        // direction vectors if nothing is passed
        // in the function to store that stuff in
        return;
    }

    const Vector3D pcv = Angle3D::cos(angles);
    const Vector3D psv = Angle3D::sin(angles);
    const Vector3D ncv = pcv * (-1.0f);
    const Vector3D nsv = psv * (-1.0f);

    Vector4D r_forward = {};

    if(forward) {
        forward[0][0] = pcv[0] * nsv[1];
        forward[0][1] = psv[0];
        forward[0][2] = pcv[0] * ncv[1];
    }

    if(right) {
        right[0][0] = pcv[2] * pcv[1];
        right[0][1] = psv[2] * pcv[1];
        right[0][2] = nsv[1];
    }

    if(up) {
        up[0][0] = psv[0] * psv[1] * pcv[2] + ncv[1] * psv[2];
        up[0][1] = pcv[0] * pcv[2];
        up[0][2] = nsv[0] * ncv[1] * pcv[2] + psv[1] * psv[2];
    }
}
