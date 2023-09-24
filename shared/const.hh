// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef EE2EDB21_A70D_41E2_949C_E378889F66DE
#define EE2EDB21_A70D_41E2_949C_E378889F66DE
#include <shared/cxmath.hh>
#include <shared/types.hh>

constexpr static const vec3f_t DIR_FORWARD = vec3f_t{0.0f, 0.0f, -1.0f};
constexpr static const vec3f_t DIR_RIGHT = vec3f_t{1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t DIR_UP = vec3f_t{0.0f, 1.0f, 0.0f};

constexpr static const vec3f_t HDIR_NORTH = vec3f_t{0.0f, 0.0f, -1.0f};
constexpr static const vec3f_t HDIR_SOUTH = vec3f_t{0.0f, 0.0f, +1.0f};
constexpr static const vec3f_t HDIR_EAST = vec3f_t{+1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t HDIR_WEST = vec3f_t{-1.0f, 0.0f, 0.0f};
constexpr static const vec3f_t HDIR_UP = vec3f_t{0.0f, +1.0f, 0.0f};
constexpr static const vec3f_t HDIR_DN = vec3f_t{0.0f, -1.0f, 0.0f};

constexpr static const size_t CHUNK_SIZE = 16;
constexpr static const size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_SIZE_LOG2 = cxmath::log2(CHUNK_SIZE);

#endif /* EE2EDB21_A70D_41E2_949C_E378889F66DE */
