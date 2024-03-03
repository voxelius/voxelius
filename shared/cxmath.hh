// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef SHARED_CXMATH_HH
#define SHARED_CXMATH_HH
#include <type_traits>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cxmath
{
template<typename T>
constexpr static inline const T degrees(const T x)
{
    return x * static_cast<T>(180.0 / M_PI);
}

template<typename T>
constexpr static inline const T radians(const T x)
{
    return x * static_cast<T>(M_PI / 180.0);
}

template<typename T>
constexpr static inline const T log2(const T x)
{
    if(x < 2)
        return 0;
    return cxmath::log2<T>((x + 1) >> 1) + 1;
}

template<typename T, typename F>
constexpr static inline const T ceil(const F x)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_floating_point_v<F>);
    const T ival = static_cast<T>(x);
    if(ival < x)
        return ival + static_cast<T>(1);
    return ival;
}

template<typename T, typename F>
constexpr static inline const T floor(const F x)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_floating_point_v<F>);
    const T ival = static_cast<T>(x);
    if(ival > x)
        return ival - static_cast<T>(1);
    return ival;
}

template<typename T>
constexpr static inline const T pow2(const T x)
{
    T value = static_cast<T>(1);
    while(value < x)
        value *= static_cast<T>(2);
    return value;
}

template<typename T>
constexpr static inline const T min(const T x, const T y)
{
    if(x > y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T max(const T x, const T y)
{
    if(x < y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T clamp(const T x, const T min, const T max)
{
    if(x < min)
        return min;
    if(x > max)
        return max;
    return x;
}

template<typename T>
constexpr static inline const T sign(const T x)
{
    if(x == static_cast<T>(0))
        return static_cast<T>(0);
    else if(std::is_signed_v<T> && (x < static_cast<T>(0)))
        return static_cast<T>(-1);
    return static_cast<T>(1);
}

template<typename T, size_t L>
constexpr static inline const size_t array_size(T(&)[L])
{
    return L;
}
} // namespace cxmath

#endif /* SHARED_CXMATH_HH */
