// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ANGLE3D_HH
#define SHARED_ANGLE3D_HH
#include <shared/vector3D.hh>

class Angle3D final : public std::array<float, 3> {
public:
    Angle3D(void) = default;
    constexpr Angle3D(const Vector3D &vector);
    constexpr Angle3D(const Angle3D &other);
    constexpr Angle3D(float pitch, float yaw, float roll);
    using std::array<float, 3>::array;

public:
    Angle3D operator+(const Angle3D &angles) const;
    Angle3D operator-(const Angle3D &angles) const;
    Angle3D operator+(const float scalar) const;
    Angle3D operator-(const float scalar) const;
    Angle3D operator*(const float scalar) const;
    Angle3D operator/(const float scalar) const;

public:
    Angle3D &operator+=(const Angle3D &angles);
    Angle3D &operator-=(const Angle3D &angles);
    Angle3D &operator+=(const float scalar);
    Angle3D &operator-=(const float scalar);
    Angle3D &operator*=(const float scalar);
    Angle3D &operator/=(const float scalar);

public:
    static Vector3D cos(const Angle3D &angles);
    static Vector3D sin(const Angle3D &angles);

public:
    static Angle3D wrap_180(const Angle3D &angles);
    static Angle3D wrap_360(const Angle3D &angles);

public:
    static void vectors(const Angle3D &angles, Vector3D *forward, Vector3D *right, Vector3D *up);
};

constexpr inline Angle3D::Angle3D(const Vector3D &vector)
{
    this[0][0] = vector[0];
    this[0][1] = vector[1];
    this[0][2] = vector[2];
}

constexpr inline Angle3D::Angle3D(const Angle3D &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline Angle3D::Angle3D(float pitch, float yaw, float roll)
{
    this[0][0] = pitch;
    this[0][1] = yaw;
    this[0][2] = roll;
}

#endif /* SHARED_ANGLE3D_HH */
