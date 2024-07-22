// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_ANGLE_HH
#define SHARED_UTIL_ANGLE_HH
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

constexpr static const float ANGLE_180D = glm::radians(180.0);
constexpr static const float ANGLE_360D = glm::radians(360.0);

namespace util
{
static inline const float wrap_180_n(const float angle)
{
    const float wrap = glm::mod(angle + ANGLE_180D, ANGLE_360D);
    return ((wrap < 0.0) ? (wrap + ANGLE_360D) : wrap) - ANGLE_180D;
}

static inline const float wrap_180_p(const float angle)
{
    return glm::mod(glm::mod(angle, ANGLE_180D) + ANGLE_180D, ANGLE_180D);
}

static inline const float wrap_360_p(const float angle)
{
    return glm::mod(glm::mod(angle, ANGLE_360D) + ANGLE_360D, ANGLE_360D);
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, float, Q> wrap_180_n(const glm::vec<L, float, Q> &angles)
{
    glm::vec<L, float, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = util::wrap_180_n(angles[i]);
    return wrap;
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, float, Q> wrap_360_p(const glm::vec<L, float, Q> &angles)
{
    glm::vec<L, float, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = util::wrap_360_p(angles[i]);
    return wrap;
}
} // namespace util

#endif /* SHARED_UTIL_ANGLE_HH */
