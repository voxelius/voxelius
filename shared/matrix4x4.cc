// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/matrix4x4.hh>
#include <utility>

Vector4D Matrix4x4::get_row(const Matrix4x4 &matrix, std::size_t row)
{
    Vector4D result = {};
    result[0] = matrix[0][row];
    result[1] = matrix[1][row];
    result[2] = matrix[2][row];
    result[3] = matrix[3][row];
    return std::move(result);
}

Matrix4x4 Matrix4x4::identity(void)
{
    Matrix4x4 result = {};
    result[0] = Vector4D(1.0f, 0.0f, 0.0f, 0.0f);
    result[1] = Vector4D(0.0f, 1.0f, 0.0f, 0.0f);
    result[2] = Vector4D(0.0f, 0.0f, 1.0f, 0.0f);
    result[3] = Vector4D(0.0f, 0.0f, 0.0f, 1.0f);
    return std::move(result);
}

Matrix4x4 Matrix4x4::psrc_view(const Vector3D &position, const Angle3D &angles)
{
    Vector3D forward, up;
    Angle3D::vectors(angles, &forward, nullptr, &up);

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

Matrix4x4 Matrix4x4::ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
    return Matrix4x4::identity();
}

Matrix4x4 Matrix4x4::persp(float yfov, float aspect, float z_near, float z_far)
{
    const float thv = std::tan(0.5f * yfov);
    Matrix4x4 result = Matrix4x4();
    result[0][0] = 1.0f / (thv * aspect);
    result[1][1] = 1.0f / (thv);
    result[2][2] = -1.0f * (z_far + z_near) / (z_far - z_near);
    result[2][3] = -1.0f;
    result[3][2] = -2.0f * z_far * z_near / (z_far - z_near);
    return std::move(result);
}

Vector4D Matrix4x4::product(const Matrix4x4 &matrix, const Vector3D &vector)
{
    const Vector4D pvec = Vector4D(vector[0], vector[1], vector[2], 1.0f);
    const Vector4D rowx = Matrix4x4::get_row(matrix, 0);
    const Vector4D rowy = Matrix4x4::get_row(matrix, 1);
    const Vector4D rowz = Matrix4x4::get_row(matrix, 2);
    const Vector4D roww = Matrix4x4::get_row(matrix, 3);

    Vector4D result = {};
    result[0] = Vector4D::dot_product(rowx, pvec);
    result[1] = Vector4D::dot_product(rowy, pvec);
    result[2] = Vector4D::dot_product(rowz, pvec);
    result[3] = Vector4D::dot_product(roww, pvec);
    return std::move(result);
}

Vector4D Matrix4x4::product(const Matrix4x4 &matrix, const Vector4D &vector)
{
    const Vector4D rowx = Matrix4x4::get_row(matrix, 0);
    const Vector4D rowy = Matrix4x4::get_row(matrix, 1);
    const Vector4D rowz = Matrix4x4::get_row(matrix, 2);
    const Vector4D roww = Matrix4x4::get_row(matrix, 3);

    Vector4D result = {};
    result[0] = Vector4D::dot_product(rowx, vector);
    result[1] = Vector4D::dot_product(rowy, vector);
    result[2] = Vector4D::dot_product(rowz, vector);
    result[3] = Vector4D::dot_product(roww, vector);
    return std::move(result);
}

Matrix4x4 Matrix4x4::product(const Matrix4x4 &amat, const Matrix4x4 &bmat)
{
    Matrix4x4 result = {};
	result[0] = amat[0] * bmat[0][0] + amat[1] * bmat[0][1] + amat[2] * bmat[0][2] + amat[3] * bmat[0][3];
	result[1] = amat[0] * bmat[1][0] + amat[1] * bmat[1][1] + amat[2] * bmat[1][2] + amat[3] * bmat[1][3];
	result[2] = amat[0] * bmat[2][0] + amat[1] * bmat[2][1] + amat[2] * bmat[2][2] + amat[3] * bmat[2][3];
	result[3] = amat[0] * bmat[3][0] + amat[1] * bmat[3][1] + amat[2] * bmat[3][2] + amat[3] * bmat[3][3];
    return std::move(result);
}
