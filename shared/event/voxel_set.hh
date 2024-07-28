#ifndef SHARED_EVENT_VOXEL_SET_HH
#define SHARED_EVENT_VOXEL_SET_HH
#include <shared/chunk_pos.hh>
#include <shared/chunk.hh>
#include <shared/local_pos.hh>
#include <shared/voxel_pos.hh>

struct VoxelSetEvent final {
    std::size_t index {};
    ChunkPos cpos {};
    LocalPos lpos {};
    VoxelPos vpos {};
    Chunk *chunk {};
    Voxel voxel {};    
};

#endif /* SHARED_EVENT_VOXEL_SET_HH */
