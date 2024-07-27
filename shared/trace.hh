// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_TRACE_HH
#define SHARED_TRACE_HH
#include <shared/chunk.hh>

struct Trace final {
public:
    EntityPos start {};
    LocalPos start_lpos {};
    VoxelPos start_vpos {};

public:
    EntityPos end {};
    LocalPos end_lpos {};
    VoxelPos end_vpos {};

public:
    float hit_distance {};
    glm::fvec3 hit_normal {};
    Chunk *hit_chunk {};
    Voxel hit_voxel {};
};

#endif /* SHARED_TRACE_HH */
