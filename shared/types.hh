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
#define M_PI 3.14159265358979323846f
#endif

using vec2f_t = glm::vec<2, float, glm::packed_highp>;
using vec3f_t = glm::vec<3, float, glm::packed_highp>;
using vec4f_t = glm::vec<4, float, glm::packed_highp>;
using mat4x4f_t = glm::mat<4, 4, float, glm::packed_highp>;
using quatf_t = glm::qua<float, glm::packed_highp>;

constexpr static const vec3f_t DIR_FORWARD = vec3f_t{0.0f, 0.0f, -1.0f};
constexpr static const vec3f_t DIR_RIGHT = vec3f_t{1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t DIR_UP = vec3f_t{0.0f, 1.0f, 0.0f};

constexpr static const vec3f_t HDIR_NORTH = vec3f_t{0.0f, 0.0f, -1.0f};
constexpr static const vec3f_t HDIR_SOUTH = vec3f_t{0.0f, 0.0f, +1.0f};
constexpr static const vec3f_t HDIR_EAST = vec3f_t{+1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t HDIR_WEST = vec3f_t{-1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t HDIR_UP = vec3f_t{0.0f, +1.0f, 0.0f};
constexpr static const vec3f_t HDIR_DN = vec3f_t{0.0f, -1.0f, 0.0f};

#endif/* EC6136EE_DE13_4F6F_98DD_B2F86921CC05 */
