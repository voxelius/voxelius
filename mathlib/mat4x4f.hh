// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/mat4x4base.hh>
#include <mathlib/vec3angles.hh>

class Mat4x4f final : public Mat4x4base<float> {
public:
    template<typename T>
    constexpr Mat4x4f(const Mat4x4base<T> &other);
    using Mat4x4base<float>::Mat4x4base;

public:
    static Mat4x4f model_rotate(const Mat4x4f &model, const float angle, const Vec3f &axis);
    static Mat4x4f proj_ortho(float left, float right, float bottom, float top, float z_near, float z_far);
    static Mat4x4f proj_persp(float y_fov, float aspect, float z_near, float z_far);
    static Mat4x4f view_psrc(const Vec3f &position, const Vec3angles &angles);
};

template<typename T>
constexpr inline Mat4x4f::Mat4x4f(const Mat4x4base<T> &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
    this[0][3] = other[3];
}
