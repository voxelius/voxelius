// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_CXMATH_HH
#define SHARED_UTIL_CXMATH_HH
#include <cmath>
#include <cstddef>
#include <type_traits>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace util
{
template<typename T>
constexpr static inline const T abs(const T x);
template<typename T, std::size_t L>
constexpr static inline const std::size_t array_size(const T(&)[L]);
template<typename T, typename F>
constexpr static inline const T ceil(const F x);
template<typename T>
constexpr static inline const T degrees(const T x);
template<typename T, typename F>
constexpr static inline const T floor(const F x);
template<typename T>
constexpr static inline const T clamp(const T x, const T min, const T max);
template<typename T, typename F>
constexpr static inline const T lerp(const T x, const T y, const F a);
template<typename T>
constexpr static inline const T log2(const T x);
template<typename T>
constexpr static inline const T max(const T x, const T y);
template<typename T>
constexpr static inline const T min(const T x, const T y);
template<typename T>
constexpr static inline const T pow2(const T x);
template<typename T>
constexpr static inline const T radians(const T x);
} // namespace util

template<typename T>
constexpr static inline const T util::abs(const T x)
{
    if(x < static_cast<T>(0))
        return -x;
    return x;
}

template<typename T, std::size_t L>
constexpr static inline const std::size_t util::array_size(const T(&)[L])
{
    return L;
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

template<typename T>
constexpr static inline const T util::degrees(const T x)
{
    return x * 180.0f / M_1_PI;
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
constexpr static inline const T util::clamp(const T x, const T min, const T max)
{
    if(x < min)
        return min;
    if(x > max)
        return max;
    return x;
}

template<typename T, typename F>
constexpr static inline const T util::lerp(const T x, const T y, const F a)
{
    static_assert(std::is_arithmetic_v<T>);
    static_assert(std::is_floating_point_v<F>);
    return static_cast<T>(static_cast<F>(x) * (static_cast<F>(1.0f) - a) + static_cast<F>(y) * a);
}

template<typename T>
constexpr static inline const T util::log2(const T x)
{
    if(x < 2)
        return 0;
    return util::log2<T>((x + 1) >> 1) + 1;
}

template<typename T>
constexpr static inline const T util::max(const T x, const T y)
{
    if(x < y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T util::min(const T x, const T y)
{
    if(x > y)
        return y;
    return x;
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
constexpr static inline const T util::radians(const T x)
{
    return x * M_PI / 180.0f;
}

#endif /* SHARED_UTIL_CXMATH_HH */
