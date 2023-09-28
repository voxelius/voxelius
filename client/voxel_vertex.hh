// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef C818F02B_A5DD_4849_955F_5D6CEAC97830
#define C818F02B_A5DD_4849_955F_5D6CEAC97830
#include <client/glxx/fwd.hh>
#include <shared/types.hh>

constexpr static const uint32_t VOXEL_VERTEX_VBO_BINDING = 0;

struct VoxelVertex final {
    uint32_t data_x {}; // 3x10-bit position
    uint32_t data_y {}; // 1x16-bit texture offset, 1x16-bit texture frame count
    vec3f_t data_norm {};
    vec2f_t data_uv {};

    VoxelVertex() = default;
    VoxelVertex(const vec3f_t &position, const vec3f_t &norm, const vec2f_t &uv, uint16_t tex_off, uint16_t tex_frames);
    static void setup(glxx::VertexArray &vao);
    static void init();
};

#endif/* C818F02B_A5DD_4849_955F_5D6CEAC97830 */
