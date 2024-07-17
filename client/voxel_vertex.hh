// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
