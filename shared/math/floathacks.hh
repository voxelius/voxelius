// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_MATH_FLOATHACKS_HH
#define SHARED_MATH_FLOATHACKS_HH
#include <cstdint>

namespace floathacks
{
static inline float int32_to_float(const std::int32_t value);
static inline float uint32_to_float(const std::uint32_t value);
static inline std::int32_t float_to_int32(const float value);
static inline std::uint32_t float_to_uint32(const float value);
} // namespace floathacks

static inline float floathacks::int32_to_float(const std::int32_t value)
{
    static_assert(sizeof(std::int32_t) == sizeof(float));
    union { std::int32_t src; float dst; } hack;
    hack.src = value;
    return hack.dst;
    
}

static inline float floathacks::uint32_to_float(const std::uint32_t value)
{
    static_assert(sizeof(std::uint32_t) == sizeof(float));
    union { std::uint32_t src; float dst; } hack;
    hack.src = value;
    return hack.dst;
}

static inline std::int32_t floathacks::float_to_int32(const float value)
{
    static_assert(sizeof(std::int32_t) == sizeof(float));
    union { float src; std::int32_t dst; } hack;
    hack.src = value;
    return hack.dst;
}

static inline std::uint32_t floathacks::float_to_uint32(const float value)
{
    static_assert(sizeof(std::uint32_t) == sizeof(float));
    union { float src; std::uint32_t dst; } hack;
    hack.src = value;
    return hack.dst;
}

#endif /* SHARED_MATH_FLOATHACKS_HH */
