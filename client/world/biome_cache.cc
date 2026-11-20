#include "client/pch.hh"

#include "client/world/biome_cache.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

void BiomeCache::init(const ChunkPos& cpos)
{
    m_biomes.fill(BIOME_ID_NULL);

    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
            auto query_pos = ChunkPos(cpos.x() + dx, cpos.y(), cpos.z() + dz);
            auto chunk = globals::world->find_chunk(query_pos);

            if(chunk == nullptr) {
                continue;
            }

            auto& chunk_registry = globals::world->chunk_registry();
            auto component = chunk_registry.try_get<BiomeSlice_Component>(chunk->entity());

            if(component == nullptr || component->ptr == nullptr) {
                continue;
            }

            const auto& src = *component->ptr;
            auto origin_x = PADDING + static_cast<std::int32_t>(dx * CHUNK_SIZE_I16);
            auto origin_z = PADDING + static_cast<std::int32_t>(dz * CHUNK_SIZE_I16);

            for(LocalPosXZ::value_type lz = 0; lz < CHUNK_SIZE_I16; lz += 1) {
                auto hz = origin_z + static_cast<std::int32_t>(lz);

                if(hz < 0 || static_cast<std::size_t>(hz) >= SIZE) {
                    continue;
                }

                auto hz_sz = static_cast<std::size_t>(hz);
                auto src_row = &src[static_cast<std::size_t>(lz) * constant::CHUNK_SIZE];

                for(LocalPosXZ::value_type lx = 0; lx < CHUNK_SIZE_I16; lx += 1) {
                    auto hx = origin_x + static_cast<std::int32_t>(lx);

                    if(hx < 0 || static_cast<std::size_t>(hx) >= SIZE) {
                        continue;
                    }

                    m_biomes[static_cast<std::size_t>(hx) + hz_sz * SIZE] = src_row[static_cast<std::size_t>(lx)];
                }
            }
        }
    }
}

biome_id_type BiomeCache::get(const LocalPosXZ& lpos) const
{
    auto hx = static_cast<std::size_t>(PADDING + lpos[0]);
    auto hz = static_cast<std::size_t>(PADDING + lpos[1]);

    if(hx >= SIZE || hz >= SIZE) {
        return BIOME_ID_NULL;
    }

    return m_biomes[hx + hz * SIZE];
}

biome_id_type BiomeCache::get(const LocalPos& lpos) const
{
    return get(LocalPosXZ(lpos.x(), lpos.z()));
}
