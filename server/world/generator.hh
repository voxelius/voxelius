#ifndef D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81
#define D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/heightmap.hh"

class ServerWorld;

template<typename T>
struct Generator final {
    static std::shared_ptr<void> attach(ServerWorld* world);
    static ColumnSlice probe(ServerWorld* world, const ChunkPosXZ& pos);
    static bool generate(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes);
};

#endif /* D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81 */
