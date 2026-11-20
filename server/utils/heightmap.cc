#include "server/pch.hh"

#include "server/utils/heightmap.hh"

#include "core/utils/modulo.hh"

#include "shared/world/world.hh"

#include "server/world/realm_surface.hh"

const ColumnSlice& utils::heightmap_probe(World* world, const ChunkPosXZ& pos)
{
    auto& heightmap = world->heightmap();
    auto entry = heightmap.get_or_create(pos);

    std::call_once(entry->init_flag, [pos, entry] {
        entry->data = realm_surface::probe(pos);
    });

    return entry->data;
}

const Column& utils::heightmap_probe_slow(World* world, const BlockPosXZ& pos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(pos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(pos[1] >> constant::CHUNK_SIZE_LOG2);

    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(pos[1], constant::CHUNK_SIZE));

    auto index = static_cast<std::size_t>(lpos[0] + lpos[1] * constant::CHUNK_SIZE);
    auto& slice = utils::heightmap_probe(world, cpos);
    return slice[index];
}
