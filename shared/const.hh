// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CONST_HH
#define SHARED_CONST_HH
#include <shared/util/cxmath.hh>
#include <shared/vector3D.hh>
#include <shared/vector4D.hh>

constexpr static float ANGLE_90D    = util::radians(90.0);
constexpr static float ANGLE_180D   = util::radians(180.0);
constexpr static float ANGLE_360D   = util::radians(360.0);

constexpr static std::size_t CHUNK_SIZE         = 16;
constexpr static std::size_t CHUNK_AREA         = CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_VOLUME       = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_SIZE_LOG2    = util::log2(CHUNK_SIZE);

constexpr static Vector3D DIR_FORWARD           = Vector3D( 0.00f,  0.00f,  1.00f);
constexpr static Vector3D DIR_BACK              = Vector3D( 0.00f,  0.00f, -1.00f);
constexpr static Vector3D DIR_LEFT              = Vector3D(-1.00f,  0.00f,  0.00f);
constexpr static Vector3D DIR_RIGHT             = Vector3D( 1.00f,  0.00f,  0.00f);
constexpr static Vector3D DIR_DOWN              = Vector3D( 0.00f, -1.00f,  0.00f);
constexpr static Vector3D DIR_UP                = Vector3D( 0.00f,  1.00f,  0.00f);

constexpr static Vector3D WDIR_NORTH            = Vector3D(DIR_FORWARD);
constexpr static Vector3D WDIR_SOUTH            = Vector3D(DIR_BACK);
constexpr static Vector3D WDIR_EAST             = Vector3D(DIR_RIGHT);
constexpr static Vector3D WDIR_WEST             = Vector3D(DIR_LEFT);
constexpr static Vector3D WDIR_DOWN             = Vector3D(DIR_DOWN);
constexpr static Vector3D WDIR_UP               = Vector3D(DIR_UP);

constexpr static Vector4D COLOR_TRANSPARENT     = Vector4D(0.00f, 0.00f, 0.00f, 0.00f);
constexpr static Vector4D COLOR_BLACK           = Vector4D(0.00f, 0.00f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_BLUE            = Vector4D(0.00f, 0.00f, 1.00f, 1.00f);
constexpr static Vector4D COLOR_GREEN           = Vector4D(0.00f, 1.00f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_CYAN            = Vector4D(0.00f, 1.00f, 1.00f, 1.00f);
constexpr static Vector4D COLOR_RED             = Vector4D(1.00f, 0.00f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_MAGENTA         = Vector4D(1.00f, 0.00f, 1.00f, 1.00f);
constexpr static Vector4D COLOR_YELLOW          = Vector4D(1.00f, 1.00f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_WHITE           = Vector4D(1.00f, 1.00f, 1.00f, 1.00f);
constexpr static Vector4D COLOR_GRAY            = Vector4D(0.50f, 0.50f, 0.50f, 1.00f);
constexpr static Vector4D COLOR_LIGHT_GRAY      = Vector4D(0.80f, 0.80f, 0.80f, 1.00f);
constexpr static Vector4D COLOR_DARK_BLUE       = Vector4D(0.00f, 0.00f, 0.50f, 1.00f);
constexpr static Vector4D COLOR_DARK_GREEN      = Vector4D(0.00f, 0.50f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_DARK_CYAN       = Vector4D(0.00f, 0.50f, 0.50f, 1.00f);
constexpr static Vector4D COLOR_DARK_RED        = Vector4D(0.50f, 0.00f, 0.00f, 1.00f);
constexpr static Vector4D COLOR_DARK_MAGENTA    = Vector4D(0.50f, 0.00f, 0.50f, 1.00f);
constexpr static Vector4D COLOR_BROWN           = Vector4D(0.50f, 0.50f, 0.00f, 1.00f);

#endif /* SHARED_CONST_HH */
