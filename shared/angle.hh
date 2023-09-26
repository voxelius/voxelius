// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B09BC94C_1FEF_49B2_8F3E_E9289AA9C836
#define B09BC94C_1FEF_49B2_8F3E_E9289AA9C836
#include <glm/common.hpp>
#include <shared/cxmath.hh>

constexpr static const float ANGLE_180D = cxmath::radians(180.0f);
constexpr static const float ANGLE_360D = cxmath::radians(360.0f);

namespace angle
{
static inline const float wrap_180_n(const float angle)
{
    const float wrap = glm::mod(angle + ANGLE_180D, ANGLE_360D);
    return ((wrap < 0.0f) ? (wrap + ANGLE_360D) : wrap) - ANGLE_180D;
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
        wrap[i] = wrap_180_n(angles[i]);
    return wrap;
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, float, Q> wrap_360_p(const glm::vec<L, float, Q> &angles)
{
    glm::vec<L, float, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = wrap_360_p(angles[i]);
    return wrap;
}
} // namespace angle

#endif/* B09BC94C_1FEF_49B2_8F3E_E9289AA9C836 */
