// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef EC6136EE_DE13_4F6F_98DD_B2F86921CC05
#define EC6136EE_DE13_4F6F_98DD_B2F86921CC05
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <inttypes.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using matrix4x4f_t = glm::mat<4, 4, float, glm::packed_highp>;
using quaternionf_t = glm::qua<float, glm::packed_highp>;
using vector2f_t = glm::vec<2, float, glm::packed_highp>;
using vector3f_t = glm::vec<3, float, glm::packed_highp>;
using vector4f_t = glm::vec<4, float, glm::packed_highp>;

using vector2u_t = glm::vec<2, uint32_t, glm::packed_highp>;
using vector3u_t = glm::vec<3, uint32_t, glm::packed_highp>;
using vector4u_t = glm::vec<4, uint32_t, glm::packed_highp>;

using quaternion_t = glm::qua<double, glm::packed_highp>;
using vector2_t = glm::vec<2, double, glm::packed_highp>;
using vector3_t = glm::vec<3, double, glm::packed_highp>;
using vector4_t = glm::vec<4, double, glm::packed_highp>;

constexpr static const vector3f_t DIR_FORWARD = vector3f_t{0.0, 0.0, -1.0};
constexpr static const vector3f_t DIR_RIGHT = vector3f_t{1.0, 0.0, 0.0};
constexpr static const vector3f_t DIR_UP = vector3f_t{0.0, 1.0, 0.0};

constexpr static const vector3f_t HDIR_NORTH = vector3f_t{0.0, 0.0, -1.0};
constexpr static const vector3f_t HDIR_SOUTH = vector3f_t{0.0, 0.0, +1.0};
constexpr static const vector3f_t HDIR_EAST = vector3f_t{+1.0, 0.0, 0.0};
constexpr static const vector3f_t HDIR_WEST = vector3f_t{-1.0, 0.0, 0.0};
constexpr static const vector3f_t HDIR_UP = vector3f_t{0.0, +1.0, 0.0};
constexpr static const vector3f_t HDIR_DN = vector3f_t{0.0, -1.0, 0.0};

#endif/* EC6136EE_DE13_4F6F_98DD_B2F86921CC05 */
