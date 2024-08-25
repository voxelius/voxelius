// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec3base.hh>
#include <mathlib/vec4base.hh>

template<typename T>
class Mat4x4base : public std::array<Vec4base<T>, 4> {
public:
    Mat4x4base(void) = default;
    constexpr Mat4x4base(const Vec4base<T> &cx, const Vec4base<T> &cy, const Vec4base<T> &cz, const Vec4base<T> &cw);
    constexpr Mat4x4base(const Mat4x4base<T> &other);
    using std::array<Vec4base<T>, 4>::array;

public:
    constexpr static Vec4base<T> product(const Mat4x4base<T> &matrix, const Vec3base<T> &vector);
    constexpr static Vec4base<T> product(const Mat4x4base<T> &matrix, const Vec4base<T> &vector);
    constexpr static Mat4x4base<T> product(const Mat4x4base<T> &amat, const Mat4x4base<T> &bmat);
public:
    constexpr static Mat4x4base<T> identity(void);
    constexpr static Mat4x4base<T> zero(void);
};

template<typename T>
constexpr inline Mat4x4base<T>::Mat4x4base(const Vec4base<T> &cx, const Vec4base<T> &cy, const Vec4base<T> &cz, const Vec4base<T> &cw)
{
    this[0][0] = cx;
    this[0][1] = cy;
    this[0][2] = cz;
    this[0][3] = cw;
}

template<typename T>
constexpr inline Mat4x4base<T>::Mat4x4base(const Mat4x4base<T> &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
    this[0][3] = other[3];
}

template<typename T>
constexpr inline Vec4base<T> Mat4x4base<T>::product(const Mat4x4base<T> &matrix, const Vec3base<T> &vector)
{
    Vec4base<T> result = {};
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2] + matrix[3][0];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2] + matrix[3][1];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2] + matrix[3][2];
    result[3] = matrix[0][3] * vector[0] + matrix[1][3] * vector[1] + matrix[2][3] * vector[2] + matrix[3][3];
    return std::move(result);
}

template<typename T>
constexpr inline Vec4base<T> Mat4x4base<T>::product(const Mat4x4base<T> &matrix, const Vec4base<T> &vector)
{
    Vec4base<T> result = {};
    result[0] = matrix[0][0] * vector[0] + matrix[1][0] * vector[1] + matrix[2][0] * vector[2] + matrix[3][0] * vector[3];
    result[1] = matrix[0][1] * vector[0] + matrix[1][1] * vector[1] + matrix[2][1] * vector[2] + matrix[3][1] * vector[3];
    result[2] = matrix[0][2] * vector[0] + matrix[1][2] * vector[1] + matrix[2][2] * vector[2] + matrix[3][2] * vector[3];
    result[3] = matrix[0][3] * vector[0] + matrix[1][3] * vector[1] + matrix[2][3] * vector[2] + matrix[3][3] * vector[3];
    return std::move(result);
}

template<typename T>
constexpr inline Mat4x4base<T> Mat4x4base<T>::product(const Mat4x4base<T> &amat, const Mat4x4base<T> &bmat)
{
    Mat4x4base<T> result = {};
	result[0] = amat[0] * bmat[0][0] + amat[1] * bmat[0][1] + amat[2] * bmat[0][2] + amat[3] * bmat[0][3];
	result[1] = amat[0] * bmat[1][0] + amat[1] * bmat[1][1] + amat[2] * bmat[1][2] + amat[3] * bmat[1][3];
	result[2] = amat[0] * bmat[2][0] + amat[1] * bmat[2][1] + amat[2] * bmat[2][2] + amat[3] * bmat[2][3];
	result[3] = amat[0] * bmat[3][0] + amat[1] * bmat[3][1] + amat[2] * bmat[3][2] + amat[3] * bmat[3][3];
    return std::move(result);
}

template<typename T>
constexpr inline Mat4x4base<T> Mat4x4base<T>::identity(void)
{
    Mat4x4base<T> result = {};
    result[0] = Vec4base<T>(T(1), T(0), T(0), T(0));
    result[1] = Vec4base<T>(T(0), T(1), T(0), T(0));
    result[2] = Vec4base<T>(T(0), T(0), T(1), T(0));
    result[3] = Vec4base<T>(T(0), T(0), T(0), T(1));
    return std::move(result);
}

template<typename T>
constexpr inline Mat4x4base<T> Mat4x4base<T>::zero(void)
{
    Mat4x4base<T> result = {};
    result[0] = Vec4base<T>(T(0), T(0), T(0), T(0));
    result[1] = Vec4base<T>(T(0), T(0), T(0), T(0));
    result[2] = Vec4base<T>(T(0), T(0), T(0), T(0));
    result[3] = Vec4base<T>(T(0), T(0), T(0), T(0));
    return std::move(result);
}
