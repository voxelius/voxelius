#include "server/pch.hh"

#include "server/utils/heightmap.hh"

#include "core/utils/modulo.hh"

#include "shared/world/world.hh"

#include "server/world/generator_map.hh"
#include "server/world/world.hh"

utils::HeightmapView::HeightmapView(ServerWorld* world, const BlockAlignedBoxXZ& bounds) : m_world(world)
{
    auto min_x = static_cast<ChunkPosXZ::value_type>(bounds.min().x() >> constant::CHUNK_SIZE_LOG2);
    auto max_x = static_cast<ChunkPosXZ::value_type>(bounds.max().x() >> constant::CHUNK_SIZE_LOG2);
    auto min_z = static_cast<ChunkPosXZ::value_type>(bounds.min().y() >> constant::CHUNK_SIZE_LOG2);
    auto max_z = static_cast<ChunkPosXZ::value_type>(bounds.max().y() >> constant::CHUNK_SIZE_LOG2);

    for(auto cx = min_x; cx <= max_x; cx += 1) {
        for(auto cz = min_z; cz <= max_z; cz += 1) {
            if(m_count == m_positions.size()) {
                return;
            }

            ChunkPosXZ pos;
            pos[0] = cx;
            pos[1] = cz;

            m_positions[m_count] = pos;
            m_slices[m_count] = &heightmap_probe(m_world, pos);
            m_count += 1;
        }
    }
}

const Column& utils::HeightmapView::at(BlockPos::value_type x, BlockPos::value_type z) const
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(x >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(z >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(mod_signed<BlockPosXZ::value_type>(x, constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(mod_signed<BlockPosXZ::value_type>(z, constant::CHUNK_SIZE));

    auto index = static_cast<std::size_t>(lpos[0] + lpos[1] * constant::CHUNK_SIZE);

    for(std::size_t i = 0; i < m_count; i += 1) {
        if(m_positions[i] == cpos) {
            return m_slices[i]->at(index);
        }
    }

    BlockPosXZ pos;
    pos[0] = x;
    pos[1] = z;

    return heightmap_probe_slow(m_world, pos);
}

const ColumnSlice& utils::heightmap_probe(ServerWorld* world, const ChunkPosXZ& pos)
{
    auto generator = world->generator();
    auto& heightmap = world->heightmap();
    auto entry = heightmap.get_or_create(pos);

    assert(generator);

    std::call_once(entry->init_flag, [pos, entry, world, generator] {
        entry->data = generator->sample_heightmap(world, pos);
    });

    return entry->data;
}

const Column& utils::heightmap_probe_slow(ServerWorld* world, const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(mod_signed<BlockPosXZ::value_type>(pos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(mod_signed<BlockPosXZ::value_type>(pos[1], constant::CHUNK_SIZE));

    auto index = static_cast<std::size_t>(lpos[0] + lpos[1] * constant::CHUNK_SIZE);
    auto& slice = heightmap_probe(world, cpos);
    return slice[index];
}
