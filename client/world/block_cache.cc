#include "client/pch.hh"

#include "client/world/block_cache.hh"

#include "shared/utils/coord.hh"
#include "shared/world/chunk.hh"
#include "shared/world/light_material.hh"
#include "shared/world/lighting.hh"
#include "shared/world/world.hh"

static std::size_t cache_index(std::size_t x, std::size_t y, std::size_t z, std::size_t size)
{
    return x + y * size + z * size * size;
}

constexpr static std::int32_t CHUNK_TOP_LOCAL_Y = static_cast<std::int32_t>(constant::CHUNK_SIZE) - 1;

// The engine has no sea level yet. It only affects the guess for chunks
// whose top layer lies below it, so one number in one place is enough.
constexpr static std::int32_t ESTIMATED_SEA_LEVEL = 0;

void BlockCache::init(const ChunkPos& cpos)
{
    m_blocks.fill(BLOCK_ID_NULL);
    m_light.fill(0x00);

    std::shared_ptr<const Chunk> chunks[3][3][3] = {};

    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
            for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
                auto delta = ChunkPos(dx, dy, dz);
                auto query_pos = cpos + delta;

                if(auto chunk = world::find_chunk(query_pos)) {
                    chunks[dx + 1][dy + 1][dz + 1] = chunk;
                }
            }
        }
    }

    constexpr auto chunk_size = static_cast<std::int32_t>(constant::CHUNK_SIZE);
    constexpr auto cache_size = static_cast<std::int32_t>(SIZE);

    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
            for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
                auto& chunk = chunks[dx + 1][dy + 1][dz + 1];

                if(chunk == nullptr) {
                    continue;
                }

                auto& blocks = chunk->blocks();
                auto light = chunk->light().data();

                // A neighbour only contributes the slice that lands inside the
                // cache, so clip the range instead of walking its whole volume
                // and throwing most of it away.
                auto hx_begin = static_cast<std::int32_t>(dx) * chunk_size + PADDING;
                auto hy_begin = static_cast<std::int32_t>(dy) * chunk_size + PADDING;
                auto hz_begin = static_cast<std::int32_t>(dz) * chunk_size + PADDING;

                auto lx_begin = std::max<std::int32_t>(0, -hx_begin);
                auto ly_begin = std::max<std::int32_t>(0, -hy_begin);
                auto lz_begin = std::max<std::int32_t>(0, -hz_begin);

                auto lx_end = std::min<std::int32_t>(chunk_size, cache_size - hx_begin);
                auto ly_end = std::min<std::int32_t>(chunk_size, cache_size - hy_begin);
                auto lz_end = std::min<std::int32_t>(chunk_size, cache_size - hz_begin);

                if(lx_begin >= lx_end || ly_begin >= ly_end || lz_begin >= lz_end) {
                    continue;
                }

                for(auto lz = lz_begin; lz < lz_end; lz += 1) {
                    for(auto ly = ly_begin; ly < ly_end; ly += 1) {
                        for(auto lx = lx_begin; lx < lx_end; lx += 1) {
                            auto f_index = utils::to_index(LocalPos(lx, ly, lz));
                            auto c_index = cache_index(static_cast<std::size_t>(hx_begin + lx), static_cast<std::size_t>(hy_begin + ly),
                                static_cast<std::size_t>(hz_begin + lz), SIZE);

                            m_blocks[c_index] = blocks.get(f_index);
                            m_light[c_index] = light[f_index];
                        }
                    }
                }
            }
        }
    }

    // Until the authoritative light arrives, approximate the sky for every
    // chunk around this one. A face samples the light of the cell outside
    // the block, so estimating only the center chunk leaves a black seam on
    // every chunk border.
    auto estimate_from_top = [&](ChunkPos::value_type dx, ChunkPos::value_type dy, ChunkPos::value_type dz) {
        auto top_y = static_cast<std::int32_t>(cpos.y() + dy) * static_cast<std::int32_t>(CHUNK_SIZE_I16) + CHUNK_TOP_LOCAL_Y;

        for(std::int32_t z = 0; z < CHUNK_SIZE_I16; ++z) {
            for(std::int32_t x = 0; x < CHUNK_SIZE_I16; ++x) {
                auto hx = static_cast<std::int32_t>(PADDING) + dx * CHUNK_SIZE_I16 + x;
                auto hz = static_cast<std::int32_t>(PADDING) + dz * CHUNK_SIZE_I16 + z;

                if(hx < 0 || hx >= static_cast<std::int32_t>(SIZE)) {
                    continue;
                }

                if(hz < 0 || hz >= static_cast<std::int32_t>(SIZE)) {
                    continue;
                }

                auto top_index = cache_index(static_cast<std::size_t>(hx),
                    static_cast<std::size_t>(PADDING + dy * CHUNK_SIZE_I16 + CHUNK_TOP_LOCAL_Y), static_cast<std::size_t>(hz), SIZE);
                auto top_id = m_blocks[static_cast<std::size_t>(top_index)];

                block_light_type level = BLOCK_LIGHT_MAX;

                if(top_y < ESTIMATED_SEA_LEVEL) {
                    auto opacity = light_material::get(top_id).opacity;
                    auto depth = static_cast<unsigned>(ESTIMATED_SEA_LEVEL - 1 - top_y);
                    auto consumed = depth * opacity;

                    level = (consumed >= BLOCK_LIGHT_MAX) ? BLOCK_LIGHT_MIN : static_cast<block_light_type>(BLOCK_LIGHT_MAX - consumed);
                }

                auto above = BLOCK_ID_NULL;

                for(std::int32_t y = CHUNK_TOP_LOCAL_Y; y >= 0; y -= 1) {
                    auto hy = static_cast<std::int32_t>(PADDING) + dy * CHUNK_SIZE_I16 + y;

                    if(hy < 0) {
                        break;
                    }

                    auto index = cache_index(static_cast<std::size_t>(hx), static_cast<std::size_t>(hy), static_cast<std::size_t>(hz),
                        SIZE);
                    auto block = m_blocks[index];
                    auto cost = light_material::crossing_cost(above, block, BLOCK_FACE_TOP);

                    level = lighting::step_level(level, cost, LIGHT_SKY, true);
                    m_light[index] = static_cast<std::uint8_t>((m_light[index] & 0x0FU) | (level << 4U));

                    above = block;
                }
            }
        }
    };

    // The chunk above only contributes its lowest cells to this cache, so its
    // own top layer is unknown here. Assume an open sky above the column.
    auto estimate_open_above = [&](ChunkPos::value_type dx, ChunkPos::value_type dz) {
        for(std::int32_t z = 0; z < CHUNK_SIZE_I16; ++z) {
            for(std::int32_t x = 0; x < CHUNK_SIZE_I16; ++x) {
                auto hx = static_cast<std::int32_t>(PADDING) + dx * CHUNK_SIZE_I16 + x;
                auto hz = static_cast<std::int32_t>(PADDING) + dz * CHUNK_SIZE_I16 + z;

                if(hx < 0 || hx >= static_cast<std::int32_t>(SIZE)) {
                    continue;
                }

                if(hz < 0 || hz >= static_cast<std::int32_t>(SIZE)) {
                    continue;
                }

                for(std::int32_t y = 0; y < CHUNK_SIZE_I16; ++y) {
                    auto hy = static_cast<std::int32_t>(PADDING) + CHUNK_SIZE_I16 + y;

                    if(hy >= static_cast<std::int32_t>(SIZE)) {
                        break;
                    }

                    auto index = cache_index(static_cast<std::size_t>(hx), static_cast<std::size_t>(hy), static_cast<std::size_t>(hz),
                        SIZE);
                    auto block = m_blocks[index];
                    auto cost = light_material::face_cost(block, BLOCK_FACE_TOP);
                    auto level = lighting::step_level(BLOCK_LIGHT_MAX, cost, LIGHT_SKY, true);

                    m_light[index] = static_cast<std::uint8_t>((m_light[index] & 0x0FU) | (level << 4U));
                }
            }
        }
    };

    for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
        for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
            for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
                auto& chunk = chunks[dx + 1][dy + 1][dz + 1];

                if(chunk == nullptr || chunk->has_light()) {
                    continue;
                }

                if(dy > 0) {
                    estimate_open_above(dx, dz);
                }
                else {
                    estimate_from_top(dx, dy, dz);
                }
            }
        }
    }
}

block_id_type BlockCache::get(const LocalPos& lpos) const
{
    auto hx = static_cast<std::size_t>(PADDING + lpos.x());
    auto hy = static_cast<std::size_t>(PADDING + lpos.y());
    auto hz = static_cast<std::size_t>(PADDING + lpos.z());

    if(hx >= SIZE || hy >= SIZE || hz >= SIZE) {
        return BLOCK_ID_NULL;
    }

    auto c_index = cache_index(hx, hy, hz, SIZE);

    return m_blocks[c_index];
}

block_light_type BlockCache::get_light(const LocalPos& lpos, light_channel channel) const
{
    auto hx = static_cast<std::size_t>(PADDING + lpos.x());
    auto hy = static_cast<std::size_t>(PADDING + lpos.y());
    auto hz = static_cast<std::size_t>(PADDING + lpos.z());

    if(hx >= SIZE || hy >= SIZE || hz >= SIZE) {
        return BLOCK_LIGHT_MIN;
    }

    auto c_index = cache_index(hx, hy, hz, SIZE);
    auto packed = m_light[c_index];

    if(channel == LIGHT_SKY) {
        return static_cast<block_light_type>(packed >> 4U);
    }

    return static_cast<block_light_type>(packed & 0x0FU);
}
