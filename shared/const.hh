// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <shared/util/cxmath.hh>

constexpr static std::size_t CHUNK_SIZE         = 16;
constexpr static std::size_t CHUNK_AREA         = CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_VOLUME       = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_SIZE_LOG2    = util::log2(CHUNK_SIZE);

constexpr static glm::fvec3 DIR_FORWARD         = glm::fvec3( 0.00f,  0.00f, -1.00f);
constexpr static glm::fvec3 DIR_BACK            = glm::fvec3( 0.00f,  0.00f,  1.00f);
constexpr static glm::fvec3 DIR_LEFT            = glm::fvec3(-1.00f,  0.00f,  0.00f);
constexpr static glm::fvec3 DIR_RIGHT           = glm::fvec3( 1.00f,  0.00f,  0.00f);
constexpr static glm::fvec3 DIR_DOWN            = glm::fvec3( 0.00f, -1.00f,  0.00f);
constexpr static glm::fvec3 DIR_UP              = glm::fvec3( 0.00f,  1.00f,  0.00f);

constexpr static glm::fvec3 WDIR_NORTH          = DIR_BACK;
constexpr static glm::fvec3 WDIR_SOUTH          = DIR_FORWARD;
constexpr static glm::fvec3 WDIR_EAST           = DIR_RIGHT;
constexpr static glm::fvec3 WDIR_WEST           = DIR_LEFT;
constexpr static glm::fvec3 WDIR_DOWN           = DIR_DOWN;
constexpr static glm::fvec3 WDIR_UP             = DIR_UP;

constexpr static glm::fvec4 COLOR_TRANSPARENT   = glm::fvec4(0.00f, 0.00f, 0.00f, 0.00f);
constexpr static glm::fvec4 COLOR_BLACK         = glm::fvec4(0.00f, 0.00f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_BLUE          = glm::fvec4(0.00f, 0.00f, 1.00f, 1.00f);
constexpr static glm::fvec4 COLOR_GREEN         = glm::fvec4(0.00f, 1.00f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_CYAN          = glm::fvec4(0.00f, 1.00f, 1.00f, 1.00f);
constexpr static glm::fvec4 COLOR_RED           = glm::fvec4(1.00f, 0.00f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_MAGENTA       = glm::fvec4(1.00f, 0.00f, 1.00f, 1.00f);
constexpr static glm::fvec4 COLOR_YELLOW        = glm::fvec4(1.00f, 1.00f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_WHITE         = glm::fvec4(1.00f, 1.00f, 1.00f, 1.00f);
constexpr static glm::fvec4 COLOR_GRAY          = glm::fvec4(0.50f, 0.50f, 0.50f, 1.00f);
constexpr static glm::fvec4 COLOR_LIGHT_GRAY    = glm::fvec4(0.80f, 0.80f, 0.80f, 1.00f);
constexpr static glm::fvec4 COLOR_DARK_BLUE     = glm::fvec4(0.00f, 0.00f, 0.50f, 1.00f);
constexpr static glm::fvec4 COLOR_DARK_GREEN    = glm::fvec4(0.00f, 0.50f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_DARK_CYAN     = glm::fvec4(0.00f, 0.50f, 0.50f, 1.00f);
constexpr static glm::fvec4 COLOR_DARK_RED      = glm::fvec4(0.50f, 0.00f, 0.00f, 1.00f);
constexpr static glm::fvec4 COLOR_DARK_MAGENTA  = glm::fvec4(0.50f, 0.00f, 0.50f, 1.00f);
constexpr static glm::fvec4 COLOR_BROWN         = glm::fvec4(0.50f, 0.50f, 0.00f, 1.00f);

#endif /* SHARED_CONST_HH */
