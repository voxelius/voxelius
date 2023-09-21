// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B9BCABB9_B3D9_477E_8A06_71E46760F32E
#define B9BCABB9_B3D9_477E_8A06_71E46760F32E
#include <type_traits>
#include <stddef.h>

namespace cxmath
{
template<typename T>
constexpr static inline const T degrees(const T x)
{
    return x * static_cast<T>(180.0f / M_PI);
}

template<typename T>
constexpr static inline const T radians(const T x)
{
    return x * static_cast<T>(M_PI / 180.0f);
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
    if(ival == static_cast<F>(ival))
        return ival;
    return ival + ((x > 0) ? 1 : 0);
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

template<typename T, size_t L>
constexpr static inline const size_t array_size(T(&)[L])
{
    return L;
}
} // namespace cxmath

#endif /* B9BCABB9_B3D9_477E_8A06_71E46760F32E */
