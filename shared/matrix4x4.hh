// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATRIX4X4_HH
#define SHARED_MATRIX4X4_HH
#include <shared/angle3D.hh>
#include <shared/vector4D.hh>

class Matrix4x4 final : public std::array<Vector4D, 4> {
public:
    // Inherit all the constructors
    using std::array<Vector4D, 4>::array;

public:
    // There is no complex matrix math going on in the project;
    // however, things like matrix multiplication actually need
    // to operate with rows of the matrix itself so get_row is used
    static Vector4D get_row(const Matrix4x4 &matrix, std::size_t row);

public:
    static Matrix4x4 identity(void);
    static Matrix4x4 psrc_view(const Vector3D &position, const Angle3D &angles);
    static Matrix4x4 ortho(float left, float right, float bottom, float top, float z_near, float z_far);
    static Matrix4x4 persp(float yfov, float aspect, float z_near, float z_far);

public:
    static Vector4D product(const Matrix4x4 &matrix, const Vector3D &vector);
    static Vector4D product(const Matrix4x4 &matrix, const Vector4D &vector);
    static Matrix4x4 product(const Matrix4x4 &amat, const Matrix4x4 &bmat);
};

#endif /* SHARED_MATRIX4X4_HH */
