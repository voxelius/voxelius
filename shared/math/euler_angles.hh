// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_EULER_ANGLES_HH
#define SHARED_MATH_EULER_ANGLES_HH
#include <shared/math/constexpr.hh>
#include <shared/math/vector3D.hh>

class EulerAngles final : public std::array<float, 3> {
public:
    EulerAngles(void) = default;
    constexpr EulerAngles(const EulerAngles &other);
    constexpr EulerAngles(float pitch, float yaw, float roll);
    using std::array<float, 3>::array;

public:
    constexpr EulerAngles operator+(const EulerAngles &angles) const;
    constexpr EulerAngles operator-(const EulerAngles &angles) const;
    constexpr EulerAngles operator+(const float scalar) const;
    constexpr EulerAngles operator-(const float scalar) const;
    constexpr EulerAngles operator*(const float scalar) const;
    constexpr EulerAngles operator/(const float scalar) const;

public:
    constexpr EulerAngles &operator+=(const EulerAngles &angles);
    constexpr EulerAngles &operator-=(const EulerAngles &angles);
    constexpr EulerAngles &operator+=(const float scalar);
    constexpr EulerAngles &operator-=(const float scalar);
    constexpr EulerAngles &operator*=(const float scalar);
    constexpr EulerAngles &operator/=(const float scalar);

public:
    static Vector3D cos(const EulerAngles &angles);
    static Vector3D sin(const EulerAngles &angles);

public:
    constexpr static EulerAngles degrees(const EulerAngles &angles);
    constexpr static EulerAngles radians(const EulerAngles &angles);
    static EulerAngles wrap_180(const EulerAngles &angles);
    static EulerAngles wrap_360(const EulerAngles &angles);

public:
    static void vectors(const EulerAngles &angles, Vector3D &forward);
    static void vectors(const EulerAngles &angles, Vector3D *forward, Vector3D *right, Vector3D *up);
};

constexpr inline EulerAngles::EulerAngles(const EulerAngles &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline EulerAngles::EulerAngles(float pitch, float yaw, float roll)
{
    this[0][0] = pitch;
    this[0][1] = yaw;
    this[0][2] = roll;
}

constexpr inline EulerAngles EulerAngles::operator+(const EulerAngles &angles) const
{
    EulerAngles result = {};
    result[0] = this[0][0] + angles[0];
    result[1] = this[0][1] + angles[1];
    result[2] = this[0][2] + angles[2];
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::operator-(const EulerAngles &angles) const
{
    EulerAngles result = {};
    result[0] = this[0][0] - angles[0];
    result[1] = this[0][1] - angles[1];
    result[2] = this[0][2] - angles[2];
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::operator+(const float scalar) const
{
    EulerAngles result = {};
    result[0] = this[0][0] + scalar;
    result[1] = this[0][1] + scalar;
    result[2] = this[0][2] + scalar;
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::operator-(const float scalar) const
{
    EulerAngles result = {};
    result[0] = this[0][0] - scalar;
    result[1] = this[0][1] - scalar;
    result[2] = this[0][2] - scalar;
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::operator*(const float scalar) const
{
    EulerAngles result = {};
    result[0] = this[0][0] * scalar;
    result[1] = this[0][1] * scalar;
    result[2] = this[0][2] * scalar;
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::operator/(const float scalar) const
{
    EulerAngles result = {};
    result[0] = this[0][0] / scalar;
    result[1] = this[0][1] / scalar;
    result[2] = this[0][2] / scalar;
    return std::move(result);
}

constexpr inline EulerAngles &EulerAngles::operator+=(const EulerAngles &angles)
{
    this[0][0] += angles[0];
    this[0][1] += angles[1];
    this[0][2] += angles[2];
    return this[0];
}

constexpr inline EulerAngles &EulerAngles::operator-=(const EulerAngles &angles)
{
    this[0][0] -= angles[0];
    this[0][1] -= angles[1];
    this[0][2] -= angles[2];
    return this[0];
}

constexpr inline EulerAngles &EulerAngles::operator+=(const float scalar)
{
    this[0][0] += scalar;
    this[0][1] += scalar;
    this[0][2] += scalar;
    return this[0];
}

constexpr inline EulerAngles &EulerAngles::operator-=(const float scalar)
{
    this[0][0] -= scalar;
    this[0][1] -= scalar;
    this[0][2] -= scalar;
    return this[0];
}

constexpr inline EulerAngles &EulerAngles::operator*=(const float scalar)
{
    this[0][0] *= scalar;
    this[0][1] *= scalar;
    this[0][2] *= scalar;
    return this[0];
}

constexpr inline EulerAngles &EulerAngles::operator/=(const float scalar)
{
    this[0][0] /= scalar;
    this[0][1] /= scalar;
    this[0][2] /= scalar;
    return this[0];
}

inline Vector3D EulerAngles::cos(const EulerAngles &angles)
{
    Vector3D result = {};
    result[0] = std::cos(angles[0]);
    result[1] = std::cos(angles[1]);
    result[2] = std::cos(angles[2]);
    return std::move(result);
}

inline Vector3D EulerAngles::sin(const EulerAngles &angles)
{
    Vector3D result = {};
    result[0] = std::sin(angles[0]);
    result[1] = std::sin(angles[1]);
    result[2] = std::sin(angles[2]);
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::degrees(const EulerAngles &angles)
{
    EulerAngles result = {};
    result[0] = cxpr::degrees(angles[0]);
    result[1] = cxpr::degrees(angles[1]);
    result[2] = cxpr::degrees(angles[2]);
    return std::move(result);
}

constexpr inline EulerAngles EulerAngles::radians(const EulerAngles &angles)
{
    EulerAngles result = {};
    result[0] = cxpr::radians(angles[0]);
    result[1] = cxpr::radians(angles[1]);
    result[2] = cxpr::radians(angles[2]);
    return std::move(result);
}

inline EulerAngles EulerAngles::wrap_180(const EulerAngles &angles)
{
    constexpr float A180 = cxpr::radians(180.0f);
    constexpr float A360 = cxpr::radians(360.0f);

    EulerAngles result = {};
    result[0] = std::fmod(angles[0] + A180, A360);
    result[1] = std::fmod(angles[1] + A180, A360);
    result[2] = std::fmod(angles[2] + A180, A360);

    result[0] = ((result[0] < 0.0f) ? (result[0] + A360) : result[0]) - A180;
    result[1] = ((result[1] < 0.0f) ? (result[1] + A360) : result[1]) - A180;
    result[2] = ((result[2] < 0.0f) ? (result[2] + A360) : result[2]) - A180;

    return std::move(result);
}

inline EulerAngles EulerAngles::wrap_360(const EulerAngles &angles)
{
    constexpr float A360 = cxpr::radians(360.0f);

    EulerAngles result = {};
    result[0] = std::fmod(std::fmod(angles[0], A360) + A360, A360);
    result[1] = std::fmod(std::fmod(angles[1], A360) + A360, A360);
    result[2] = std::fmod(std::fmod(angles[2], A360) + A360, A360);
    return std::move(result);
}

inline void EulerAngles::vectors(const EulerAngles &angles, Vector3D &forward)
{
    const float cosp = std::cos(angles[0]);
    const float cosy = std::cos(angles[1]);
    const float sinp = std::sin(angles[0]);
    const float siny = std::sin(angles[1]);

    forward[0] = cosp * siny * (-1.0f);
    forward[1] = sinp;
    forward[2] = cosp * cosy * (-1.0f);
}

inline void EulerAngles::vectors(const EulerAngles &angles, Vector3D *forward, Vector3D *right, Vector3D *up)
{
    if(!forward && !right && !up) {
        // There's no point in figuring out
        // direction vectors if nothing is passed
        // in the function to store that stuff in
        return;
    }

    const Vector3D pcv = EulerAngles::cos(angles);
    const Vector3D psv = EulerAngles::sin(angles);
    const Vector3D ncv = pcv * (-1.0f);
    const Vector3D nsv = psv * (-1.0f);

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

#endif /* SHARED_MATH_EULER_ANGLES_HH */
