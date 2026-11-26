#ifndef D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81
#define D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/heightmap.hh"

class ServerWorld;
struct WorldDefinition;

template<typename T>
struct Generator final {
    static std::shared_ptr<void> create_state(ryml::ConstNodeRef settings, const WorldDefinition& definition, std::string_view where);
    static ColumnSlice sample_heightmap(ServerWorld* world, const ChunkPosXZ& pos);
    static bool generate_blocks(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes);
    static void generate_biomes(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out);
    static block_id_type surface_block(ServerWorld* world, const BlockPosXZ& pos, const Column& column);
};

#endif /* D5A7C9E1_3F2B_4D6A_8C0E_1B3D5F7A9C81 */
