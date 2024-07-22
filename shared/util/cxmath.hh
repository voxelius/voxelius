// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_CXMATH_HH
#define SHARED_UTIL_CXMATH_HH
#include <cstddef>
#include <type_traits>

namespace util
{
template<typename T>
constexpr static inline const T log2(const T x);
template<typename T, typename F>
constexpr static inline const T ceil(const F x);
template<typename T, typename F>
constexpr static inline const T floor(const F x);
template<typename T>
constexpr static inline const T pow2(const T x);
template<typename T>
constexpr static inline const T min(const T x, const T y);
template<typename T>
constexpr static inline const T max(const T x, const T y);
template<typename T>
constexpr static inline const T clamp(const T x, const T min, const T max);
template<typename T, size_t L>
constexpr static inline const size_t array_size(T(&)[L]);
} // namespace util

template<typename T>
constexpr static inline const T util::log2(const T x)
{
    if(x < 2)
        return 0;
    return util::log2<T>((x + 1) >> 1) + 1;
}

template<typename T, typename F>
constexpr static inline const T util::ceil(const F x)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_floating_point_v<F>);

    const T ival = static_cast<T>(x);
    if(ival < x)
        return ival + static_cast<T>(1);
    return ival;
}

template<typename T, typename F>
constexpr static inline const T util::floor(const F x)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_floating_point_v<F>);

    const T ival = static_cast<T>(x);
    if(ival > x)
        return ival - static_cast<T>(1);
    return ival;
}

template<typename T>
constexpr static inline const T util::pow2(const T x)
{
    T value = static_cast<T>(1);
    while(value < x)
        value *= static_cast<T>(2);
    return value;
}

template<typename T>
constexpr static inline const T util::min(const T x, const T y)
{
    if(x > y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T util::max(const T x, const T y)
{
    if(x < y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T util::clamp(const T x, const T min, const T max)
{
    if(x < min)
        return min;
    if(x > max)
        return max;
    return x;
}

template<typename T, size_t L>
constexpr static inline const size_t util::array_size(T(&)[L])
{
    return L;
}

#endif /* SHARED_UTIL_CXMATH_HH */
