// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef EE2EDB21_A70D_41E2_949C_E378889F66DE
#define EE2EDB21_A70D_41E2_949C_E378889F66DE
#include <game/shared/cxmath.hh>
#include <glm/vec3.hpp>

constexpr static const glm::vec3 DIR_FORWARD = glm::vec3{0.0f, 0.0f, -1.0f};
constexpr static const glm::vec3 DIR_RIGHT = glm::vec3{1.0f, 0.0f, 0.0f};
constexpr static const glm::vec3 DIR_UP = glm::vec3{0.0f, 1.0f, 0.0f};

constexpr static const glm::vec3 HDIR_NORTH = glm::vec3{0.0f, 0.0f, -1.0f};
constexpr static const glm::vec3 HDIR_SOUTH = glm::vec3{0.0f, 0.0f, +1.0f};
constexpr static const glm::vec3 HDIR_EAST = glm::vec3{+1.0f, 0.0f, 0.0f};
constexpr static const glm::vec3 HDIR_WEST = glm::vec3{-1.0f, 0.0f, 0.0f};
constexpr static const glm::vec3 HDIR_UP = glm::vec3{0.0f, +1.0f, 0.0f};
constexpr static const glm::vec3 HDIR_DN = glm::vec3{0.0f, -1.0f, 0.0f};

constexpr static const size_t CHUNK_SIZE = 16;
constexpr static const size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_SIZE_LOG2 = cxmath::log2(CHUNK_SIZE);

#endif /* EE2EDB21_A70D_41E2_949C_E378889F66DE */
