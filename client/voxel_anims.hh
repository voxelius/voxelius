// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VOXEL_ANIMS_HH
#define CLIENT_VOXEL_ANIMS_HH
#include <cstdint>
#include <glad/gl.h>

namespace voxel_anims
{
extern unsigned int fps;
} // namespace voxel_anims

namespace voxel_anims
{
extern std::uint64_t nextframe;
extern std::uint32_t frame;
} // namespace voxel_anims

namespace voxel_anims
{
void init(void);
void deinit(void);
void update(void);
} // namespace voxel_anims

namespace voxel_anims
{
void construct(void);
GLuint buffer(void);
} // namespace voxel_anims

#endif /* CLIENT_VOXEL_ANIMS_HH */
