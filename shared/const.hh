// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <shared/cxmath.hh>

constexpr static const size_t CHUNK_SIZE    = 16;
constexpr static const size_t CHUNK_AREA    = CHUNK_SIZE * CHUNK_SIZE;
constexpr static const size_t CHUNK_VOLUME  = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

constexpr static const size_t CHUNK_SIZE_LOG2 = cxmath::log2(CHUNK_SIZE);

constexpr static const glm::dvec3 DIR_FORWARD   = {0.000, 0.000, -1.000};
constexpr static const glm::dvec3 DIR_RIGHT     = {1.000, 0.000,  0.000};
constexpr static const glm::dvec3 DIR_UP        = {0.000, 1.000,  0.000};

constexpr static const glm::fvec3 FDIR_FORWARD  = {0.000f, 0.000f, -1.000f};
constexpr static const glm::fvec3 FDIR_RIGHT    = {1.000f, 0.000f,  0.000f};
constexpr static const glm::fvec3 FDIR_UP       = {0.000f, 1.000f,  0.000f};

constexpr static const glm::dvec3 HDIR_NORTH    = { 0.000,  0.000, -1.000};
constexpr static const glm::dvec3 HDIR_SOUTH    = { 0.000,  0.000,  1.000};
constexpr static const glm::dvec3 HDIR_EAST     = { 1.000,  0.000,  0.000};
constexpr static const glm::dvec3 HDIR_WEST     = {-1.000,  0.000,  0.000};
constexpr static const glm::dvec3 HDIR_UP       = { 0.000,  1.000,  0.000};
constexpr static const glm::dvec3 HDIR_DN       = { 0.000, -1.000,  0.000};

constexpr static const glm::dvec4 COLOR_TRANSPARENT     = {0.000, 0.000, 0.000, 0.000};
constexpr static const glm::dvec4 COLOR_BLACK           = {0.000, 0.000, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_BLUE            = {0.000, 0.000, 1.000, 1.000};
constexpr static const glm::dvec4 COLOR_GREEN           = {0.000, 1.000, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_CYAN            = {0.000, 1.000, 1.000, 1.000};
constexpr static const glm::dvec4 COLOR_RED             = {1.000, 0.000, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_MAGENTA         = {1.000, 0.000, 1.000, 1.000};
constexpr static const glm::dvec4 COLOR_YELLOW          = {1.000, 1.000, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_WHITE           = {1.000, 1.000, 1.000, 1.000};
constexpr static const glm::dvec4 COLOR_GRAY            = {0.500, 0.500, 0.500, 1.000};
constexpr static const glm::dvec4 COLOR_LIGHT_GRAY      = {0.800, 0.800, 0.800, 1.000};
constexpr static const glm::dvec4 COLOR_DARK_BLUE       = {0.000, 0.000, 0.500, 1.000};
constexpr static const glm::dvec4 COLOR_DARK_GREEN      = {0.000, 0.500, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_DARK_CYAN       = {0.000, 0.500, 0.500, 1.000};
constexpr static const glm::dvec4 COLOR_DARK_RED        = {0.500, 0.000, 0.000, 1.000};
constexpr static const glm::dvec4 COLOR_DARK_MAGENTA    = {0.500, 0.000, 0.500, 1.000};
constexpr static const glm::dvec4 COLOR_BROWN           = {0.500, 0.500, 0.000, 1.000};

#endif /* SHARED_CONST_HH */
