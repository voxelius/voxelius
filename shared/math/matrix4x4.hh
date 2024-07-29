// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATRIX4X4_HH
#define SHARED_MATRIX4X4_HH
#include <shared/math/euler_angles.hh>
#include <shared/math/vector4D.hh>

class Matrix4x4 final : public std::array<Vector4D, 4> {
public:
    Matrix4x4(void) = default;
    constexpr Matrix4x4(const Vector4D &cx, const Vector4D &cy, const Vector4D &cz, const Vector4D &cw);
    constexpr Matrix4x4(const Matrix4x4 &other);
    using std::array<Vector4D, 4>::array;

public:
    static Matrix4x4 proj_ortho(float left, float right, float bottom, float top, float z_near, float z_far);
    static Matrix4x4 proj_persp(float y_fov, float aspect, float z_near, float z_far);
    static Matrix4x4 view_psrc(const Vector3D &position, const EulerAngles &angles);

public:
    constexpr static Vector4D product(const Matrix4x4 &matrix, const Vector3D &vector);
    constexpr static Vector4D product(const Matrix4x4 &matrix, const Vector4D &vector);
    constexpr static Matrix4x4 product(const Matrix4x4 &amat, const Matrix4x4 &bmat);

public:
    constexpr static Matrix4x4 identity(void);
    constexpr static Matrix4x4 zero(void);
};

constexpr inline Matrix4x4::Matrix4x4(const Vector4D &cx, const Vector4D &cy, const Vector4D &cz, const Vector4D &cw)
{
    this[0][0] = cx;
    this[0][1] = cy;
    this[0][2] = cz;
    this[0][3] = cw;
}

constexpr inline Matrix4x4::Matrix4x4(const Matrix4x4 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
    this[0][3] = other[3];
}

inline Matrix4x4 Matrix4x4::proj_ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
    // UNDONE: implement ortho
    return Matrix4x4::identity();
}

inline Matrix4x4 Matrix4x4::proj_persp(float y_fov, float aspect, float z_near, float z_far)
{
    const float inv_z_delta = 1.0f / (z_far - z_near);
    const float tan_half_fov = std::tan(0.5f * y_fov);

    Matrix4x4 result = Matrix4x4::zero();
    result[0][0] = 1.0f / (aspect * tan_half_fov);
    result[1][1] = 1.0f / (tan_half_fov);
    result[2][2] = -1.0f * (z_far + z_near) * inv_z_delta;
    result[2][3] = -1.0f;
    result[3][2] = -2.0f * z_far * z_near * inv_z_delta;
    return std::move(result);
}

inline Matrix4x4 Matrix4x4::view_psrc(const Vector3D &position, const EulerAngles &angles)
{
    Vector3D forward, up;
    EulerAngles::vectors(angles, &forward, nullptr, &up);

    Matrix4x4 result = Matrix4x4::identity();
    result[0][0] = forward[1] * up[2] - forward[2] * up[1];
    result[1][0] = forward[2] * up[0] - forward[0] * up[2];
    result[2][0] = forward[0] * up[1] - forward[1] * up[0];
    result[3][0] = -result[0][0] * position[0] - result[1][0] * position[1] - result[2][0] * position[2];
    result[0][1] = up[0];
    result[1][1] = up[1];
    result[2][1] = up[2];
    result[3][1] = -up[0] * position[0] - up[1] * position[1] - up[2] * position[2];
    result[0][2] = -forward[0];
    result[1][2] = -forward[1];
    result[2][2] = -forward[2];
    result[3][2] = forward[0] * position[0] + forward[1] * position[1] + forward[2] * position[2];
    return std::move(result);
}

constexpr inline Vector4D Matrix4x4::product(const Matrix4x4 &matrix, const Vector3D &vector)
{
    Vector4D result = {};
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2] + matrix[3][0];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2] + matrix[3][1];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2] + matrix[3][2];
    result[3] = matrix[0][3] * vector[0] + matrix[1][3] * vector[1] + matrix[2][3] * vector[2] + matrix[3][3];
    return std::move(result);
}

constexpr inline Vector4D Matrix4x4::product(const Matrix4x4 &matrix, const Vector4D &vector)
{
    Vector4D result = {};
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2] + matrix[3][0] * vector[3];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2] + matrix[3][1] * vector[3];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2] + matrix[3][2] * vector[3];
    result[3] = matrix[0][3] * vector[0] + matrix[1][3] * vector[1] + matrix[2][3] * vector[2] + matrix[3][3] * vector[3];
    return std::move(result);
}

constexpr inline Matrix4x4 Matrix4x4::product(const Matrix4x4 &amat, const Matrix4x4 &bmat)
{
    Matrix4x4 result = {};
	result[0] = amat[0] * bmat[0][0] + amat[1] * bmat[0][1] + amat[2] * bmat[0][2] + amat[3] * bmat[0][3];
	result[1] = amat[0] * bmat[1][0] + amat[1] * bmat[1][1] + amat[2] * bmat[1][2] + amat[3] * bmat[1][3];
	result[2] = amat[0] * bmat[2][0] + amat[1] * bmat[2][1] + amat[2] * bmat[2][2] + amat[3] * bmat[2][3];
	result[3] = amat[0] * bmat[3][0] + amat[1] * bmat[3][1] + amat[2] * bmat[3][2] + amat[3] * bmat[3][3];
    return std::move(result);
}

constexpr inline Matrix4x4 Matrix4x4::identity(void)
{
    Matrix4x4 result = {};
    result[0] = Vector4D(1.0f, 0.0f, 0.0f, 0.0f);
    result[1] = Vector4D(0.0f, 1.0f, 0.0f, 0.0f);
    result[2] = Vector4D(0.0f, 0.0f, 1.0f, 0.0f);
    result[3] = Vector4D(0.0f, 0.0f, 0.0f, 1.0f);
    return std::move(result);
}

constexpr inline Matrix4x4 Matrix4x4::zero(void)
{
    Matrix4x4 result = {};
    result[0] = Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
    result[1] = Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
    result[2] = Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
    result[3] = Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
    return std::move(result);
}

#endif /* SHARED_MATRIX4X4_HH */
