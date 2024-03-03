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
#ifndef CLIENT_VOXEL_VERTEX_HH
#define CLIENT_VOXEL_VERTEX_HH
#include <client/glxx/vertexarray.hh>
#include <shared/const.hh>

constexpr static const uint32_t VOXEL_VBO_BINDING = 0;

struct VoxelVertex final {
    // vvdat_i[0]: 3x10-bit vertex position, 1x2-bit vertex shade
    // vvdat_i[1]: 1x16-bit toffset, 1x16-bit tframes
    // vvdat_i[2]: 2x16-bit texture coordinates
    uint32_t vvdat_i[3] {};

    VoxelVertex() = default;
    VoxelVertex(const glm::dvec3 &position, unsigned int shade, uint16_t toffset, uint16_t tframes, const glm::dvec2 &uv);
    static void setup(glxx::VertexArray &vao);
    static void init();
};

#endif /* CLIENT_VOXEL_VERTEX_HH */
