// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

constexpr static size_t CHUNK_SIZE      = 16;
constexpr static size_t CHUNK_AREA      = CHUNK_SIZE * CHUNK_SIZE;
constexpr static size_t CHUNK_VOLUME    = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static size_t CHUNK_BIT_SHIFT = 4;

constexpr static glm::dvec3 DIR_FORWARD = { 0.00,  0.00, -1.00};
constexpr static glm::dvec3 DIR_BACK    = { 0.00,  0.00,  1.00};
constexpr static glm::dvec3 DIR_LEFT    = {-1.00,  0.00,  0.00};
constexpr static glm::dvec3 DIR_RIGHT   = { 1.00,  0.00,  0.00};
constexpr static glm::dvec3 DIR_DOWN    = { 0.00, -1.00,  0.00};
constexpr static glm::dvec3 DIR_UP      = { 0.00,  1.00,  0.00};

constexpr static glm::dvec3 WDIR_NORTH  = DIR_FORWARD;
constexpr static glm::dvec3 WDIR_SOUTH  = DIR_BACK;
constexpr static glm::dvec3 WDIR_EAST   = DIR_RIGHT;
constexpr static glm::dvec3 WDIR_WEST   = DIR_LEFT;

constexpr static glm::dvec4 COLOR_TRANSPARENT   = {0.00, 0.00, 0.00, 0.00};
constexpr static glm::dvec4 COLOR_BLACK         = {0.00, 0.00, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_BLUE          = {0.00, 0.00, 1.00, 1.00};
constexpr static glm::dvec4 COLOR_GREEN         = {0.00, 1.00, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_CYAN          = {0.00, 1.00, 1.00, 1.00};
constexpr static glm::dvec4 COLOR_RED           = {1.00, 0.00, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_MAGENTA       = {1.00, 0.00, 1.00, 1.00};
constexpr static glm::dvec4 COLOR_YELLOW        = {1.00, 1.00, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_WHITE         = {1.00, 1.00, 1.00, 1.00};
constexpr static glm::dvec4 COLOR_GRAY          = {0.50, 0.50, 0.50, 1.00};
constexpr static glm::dvec4 COLOR_LIGHT_GRAY    = {0.80, 0.80, 0.80, 1.00};
constexpr static glm::dvec4 COLOR_DARK_BLUE     = {0.00, 0.00, 0.50, 1.00};
constexpr static glm::dvec4 COLOR_DARK_GREEN    = {0.00, 0.50, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_DARK_CYAN     = {0.00, 0.50, 0.50, 1.00};
constexpr static glm::dvec4 COLOR_DARK_RED      = {0.50, 0.00, 0.00, 1.00};
constexpr static glm::dvec4 COLOR_DARK_MAGENTA  = {0.50, 0.00, 0.50, 1.00};
constexpr static glm::dvec4 COLOR_BROWN         = {0.50, 0.50, 0.00, 1.00};

#endif /* SHARED_CONST_HH */