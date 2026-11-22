#ifndef F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11
#define F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11

#include "server/world/generator.hh"

struct GeneratorDefinition final {
    std::shared_ptr<void> (*create_state)(lua_State* L, int settings_idx, const WorldDefinition& definition);
    ColumnSlice (*sample_heightmap)(ServerWorld* world, const ChunkPosXZ& pos);
    bool (*generate_blocks)(ServerWorld* world, const ChunkPos& pos, BlockStorage& blocks, BiomeSlice& biomes);
    void (*generate_biomes)(ServerWorld* world, const ChunkPosXZ& pos, BiomeSlice& out);
};

namespace generator_map::detail
{
void add(std::string_view name, GeneratorDefinition definition);
} // namespace generator_map::detail

namespace generator_map
{
template<typename T>
void add(std::string_view name);
} // namespace generator_map

namespace generator_map
{
const GeneratorDefinition* find(std::string_view name);
} // namespace generator_map

template<typename T>
void generator_map::add(std::string_view name)
{
    GeneratorDefinition definition {};
    definition.create_state = &Generator<T>::create_state;
    definition.sample_heightmap = &Generator<T>::sample_heightmap;
    definition.generate_blocks = &Generator<T>::generate_blocks;
    definition.generate_biomes = &Generator<T>::generate_biomes;

    detail::add(name, definition);
}

#endif /* F0A5E0C4_0C55_4A0B_9A3B_0F5D1A7B2C11 */
