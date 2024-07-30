// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/mat4x4f.hh>

Mat4x4f Mat4x4f::proj_ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
    // UNDONE: implement ortho
    return Mat4x4f::identity();
}

Mat4x4f Mat4x4f::proj_persp(float y_fov, float aspect, float z_near, float z_far)
{
    const float inv_z_delta = 1.0f / (z_far - z_near);
    const float tan_half_fov = std::tan(0.5f * y_fov);

    Mat4x4f result = Mat4x4f::zero();
    result[0][0] = 1.0f / (aspect * tan_half_fov);
    result[1][1] = 1.0f / (tan_half_fov);
    result[2][2] = -1.0f * (z_far + z_near) * inv_z_delta;
    result[2][3] = -1.0f;
    result[3][2] = -2.0f * z_far * z_near * inv_z_delta;
    return std::move(result);
}

Mat4x4f Mat4x4f::view_psrc(const Vec3f &position, const Vec3angles &angles)
{
    Vec3f forward, up;
    Vec3angles::vectors(angles, &forward, nullptr, &up);

    Mat4x4f result = Mat4x4f::identity();
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
