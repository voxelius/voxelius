#ifndef SHARED_EVENT_VOXEL_SET_HH
#define SHARED_EVENT_VOXEL_SET_HH
#include <shared/chunk.hh>

struct VoxelSetEvent final {
    std::size_t index {};
    ChunkCoord cpos {};
    LocalCoord lpos {};
    VoxelCoord vpos {};
    Chunk *chunk {};
    Voxel voxel {};    
};

#endif /* SHARED_EVENT_VOXEL_SET_HH */
