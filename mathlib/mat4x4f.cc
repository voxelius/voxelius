// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <mathlib/mat4x4f.hh>

Mat4x4f Mat4x4f::model_rotate(const Mat4x4f &model, const float angle, const Vec3f &axis)
{
    const float cv = std::cosf(angle);
    const float sv = std::sinf(angle);
    
    const Vec3f ax = Vec3f::normalized(axis);
    const Vec3f tm = Vec3f(ax * (1.0f - cv));
    
    Mat4x4f rotmat = Mat4x4f::zero();
	rotmat[0][0] = cv + tm[0] * ax[0];
	rotmat[0][1] = tm[0] * ax[1] + sv * ax[2];
	rotmat[0][2] = tm[0] * ax[2] - sv * ax[1];
	rotmat[1][0] = tm[1] * ax[0] - sv * ax[2];
	rotmat[1][1] = cv + tm[1] * ax[1];
	rotmat[1][2] = tm[1] * ax[2] + sv * ax[0];
	rotmat[2][0] = tm[2] * ax[0] + sv * ax[1];
	rotmat[2][1] = tm[2] * ax[1] - sv * ax[0];
	rotmat[2][2] = cv + tm[2] * ax[2];

    Mat4x4f result = Mat4x4f::zero();
    result[0] = model[0] * rotmat[0][0] + model[1] * rotmat[0][1] + model[2] * rotmat[0][2];
    result[1] = model[0] * rotmat[1][0] + model[1] * rotmat[1][1] + model[2] * rotmat[1][2];
    result[2] = model[0] * rotmat[2][0] + model[1] * rotmat[2][1] + model[2] * rotmat[2][2];
    result[3] = model[3];
    return std::move(result);
}

Mat4x4f Mat4x4f::proj_ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
    Mat4x4f result = Mat4x4f::identity();
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 1.0f / (z_far - z_near);
    result[3][0] = -1.0f * (right + left) / (right - left);
    result[3][1] = -1.0f * (top + bottom) / (top - bottom);
    result[3][2] = -1.0f * z_near / (z_far - z_near);
    return std::move(result);
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
