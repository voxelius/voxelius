// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_ANGLE_HH
#define SHARED_ANGLE_HH
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

constexpr static const double ANGLE_180D = glm::radians(180.0);
constexpr static const double ANGLE_360D = glm::radians(360.0);

namespace angle
{
static inline const double wrap_180_n(const double angle)
{
    const double wrap = glm::mod(angle + ANGLE_180D, ANGLE_360D);
    return ((wrap < 0.0) ? (wrap + ANGLE_360D) : wrap) - ANGLE_180D;
}

static inline const double wrap_180_p(const double angle)
{
    return glm::mod(glm::mod(angle, ANGLE_180D) + ANGLE_180D, ANGLE_180D);
}

static inline const double wrap_360_p(const double angle)
{
    return glm::mod(glm::mod(angle, ANGLE_360D) + ANGLE_360D, ANGLE_360D);
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, double, Q> wrap_180_n(const glm::vec<L, double, Q> &angles)
{
    glm::vec<L, double, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = angle::wrap_180_n(angles[i]);
    return wrap;
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, double, Q> wrap_360_p(const glm::vec<L, double, Q> &angles)
{
    glm::vec<L, double, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = angle::wrap_360_p(angles[i]);
    return wrap;
}
} // namespace angle

#endif /* SHARED_ANGLE_HH */
