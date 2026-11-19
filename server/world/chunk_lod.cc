#include "server/pch.hh"

#include "server/world/chunk_lod.hh"

#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/block_storage.hh"
#include "shared/world/chunk.hh"
#include "shared/world/lod.hh"
#include "shared/world/world.hh"

#include "server/world/lod_color.hh"

constexpr static std::size_t MAX_REBUILDS_PER_TICK = 4;

constexpr static std::array<std::size_t, 5> LEVEL_OFFSETS = { 0, 4096, 4608, 4672, 4680 };
constexpr static std::array<std::int32_t, 5> LEVEL_DIMS = { 16, 8, 4, 2, 1 };

struct RawCell final {
    Eigen::Vector3f color;
    std::uint8_t coverage_flags;
};

static std::size_t block_index(std::int32_t x, std::int32_t y, std::int32_t z)
{
    auto index = (y * constant::CHUNK_SIZE + z) * constant::CHUNK_SIZE + x;
    return static_cast<std::size_t>(index);
}

static std::size_t cell_index(std::int32_t dim, std::int32_t x, std::int32_t y, std::int32_t z)
{
    auto index = (y * dim + z) * dim + x;
    return static_cast<std::size_t>(index);
}

static void sample_block_cell(std::int32_t x, std::int32_t y, std::int32_t z, std::span<const bool> solid, std::span<const bool> water,
    std::span<const Eigen::Vector3f> colors, RawCell& cell)
{
    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    auto solid_count = 0;
    auto water_count = 0;

    for(std::int32_t dy = 0; dy < 2; dy += 1) {
        for(std::int32_t dz = 0; dz < 2; dz += 1) {
            for(std::int32_t dx = 0; dx < 2; dx += 1) {
                auto index = block_index(x * 2 + dx, y * 2 + dy, z * 2 + dz);

                if(!solid[index]) {
                    continue;
                }

                sum += colors[index];
                solid_count += 1;

                if(water[index]) {
                    water_count += 1;
                }
            }
        }
    }

    if(solid_count == 0) {
        cell.color = Eigen::Vector3f::Zero();
        cell.coverage_flags = 0;
        return;
    }

    cell.color = sum / static_cast<float>(solid_count);
    cell.coverage_flags = static_cast<std::uint8_t>((solid_count * 15 + 4) / 8);

    if(water_count * 2 >= solid_count) {
        cell.coverage_flags |= CHUNK_LOD_WATER_BIT;
    }
}

static void sample_child_cell(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t child_dim, std::span<const RawCell> children,
    RawCell& cell)
{
    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    auto weight = 0;
    auto coverage_sum = 0;
    auto water_coverage = 0;

    for(std::int32_t dy = 0; dy < 2; dy += 1) {
        for(std::int32_t dz = 0; dz < 2; dz += 1) {
            for(std::int32_t dx = 0; dx < 2; dx += 1) {
                auto& child = children[cell_index(child_dim, x * 2 + dx, y * 2 + dy, z * 2 + dz)];
                auto coverage = child.coverage_flags & CHUNK_LOD_COVERAGE_MASK;

                coverage_sum += coverage;

                if(child.coverage_flags & CHUNK_LOD_WATER_BIT) {
                    water_coverage += coverage;
                }

                if(coverage) {
                    sum += child.color * static_cast<float>(coverage);
                    weight += coverage;
                }
            }
        }
    }

    if(weight) {
        cell.color = sum / static_cast<float>(weight);
    }
    else {
        cell.color = Eigen::Vector3f::Zero();
    }

    cell.coverage_flags = static_cast<std::uint8_t>((coverage_sum + 4) / 8);

    if(coverage_sum && water_coverage * 2 >= coverage_sum) {
        cell.coverage_flags |= CHUNK_LOD_WATER_BIT;
    }
}

static void build_level_1(const BlockStorage& blocks, std::span<const biome_id_type> biomes, std::span<RawCell> raw)
{
    thread_local std::array<bool, constant::CHUNK_VOLUME> solid;
    thread_local std::array<bool, constant::CHUNK_VOLUME> water;
    thread_local std::array<Eigen::Vector3f, constant::CHUNK_VOLUME> colors;

    solid.fill(false);
    water.fill(false);
    colors.fill(Eigen::Vector3f::Zero());

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto id = blocks.get(i);

        if(id == BLOCK_ID_NULL) {
            continue;
        }

        auto def = block_registry::find_definition(id);

        if(def == nullptr) {
            continue;
        }

        auto is_fluid = def->fluid && def->fluid_level > 0;

        if(def->render == BLOCK_RENDER_NONE && !is_fluid) {
            continue;
        }

        auto lpos = utils::to_local(i);
        auto biome_index = static_cast<std::size_t>(lpos.x() + lpos.z() * constant::CHUNK_SIZE);
        auto biome = biome_index < biomes.size() ? biomes[biome_index] : BIOME_ID_NULL;

        solid[i] = true;
        water[i] = is_fluid;
        colors[i] = lod_color::block_color(id, biome);
    }

    for(std::size_t i = 0; i < constant::CHUNK_VOLUME; ++i) {
        auto& cell = raw[i];

        if(!solid[i]) {
            cell.color = Eigen::Vector3f::Zero();
            cell.coverage_flags = 0;
            continue;
        }

        cell.color = colors[i];
        cell.coverage_flags = 15;

        if(water[i]) {
            cell.coverage_flags |= CHUNK_LOD_WATER_BIT;
        }
    }

    for(std::int32_t y = 0; y < 8; ++y) {
        for(std::int32_t z = 0; z < 8; ++z) {
            for(std::int32_t x = 0; x < 8; ++x) {
                auto index = LEVEL_OFFSETS[1] + cell_index(8, x, y, z);
                sample_block_cell(x, y, z, solid, water, colors, raw[index]);
            }
        }
    }
}

static void build_levels(std::span<RawCell> raw)
{
    for(std::uint8_t level = 2; level <= CHUNK_LOD_LEVEL_MAX; level += 1) {
        auto dim = LEVEL_DIMS[level];
        auto child_dim = LEVEL_DIMS[level - 1];
        auto offset = LEVEL_OFFSETS[level];
        auto child_offset = LEVEL_OFFSETS[level - 1];

        auto children = raw.subspan(child_offset);

        for(std::int32_t y = 0; y < dim; ++y) {
            for(std::int32_t z = 0; z < dim; ++z) {
                for(std::int32_t x = 0; x < dim; ++x) {
                    auto index = offset + cell_index(dim, x, y, z);
                    sample_child_cell(x, y, z, child_dim, children, raw[index]);
                }
            }
        }
    }
}

static bool pack_lod(const std::array<RawCell, CHUNK_LOD_TOTAL_CELLS>& raw, ChunkLod& lod)
{
    auto any_solid = false;

    for(std::size_t i = 0; i < CHUNK_LOD_TOTAL_CELLS; ++i) {
        if(raw[i].coverage_flags & CHUNK_LOD_COVERAGE_MASK) {
            any_solid = true;
            break;
        }
    }

    if(!any_solid) {
        return false;
    }

    vx::hash_set<std::uint32_t> unique;
    auto overflow = false;

    for(std::size_t i = 0; i < CHUNK_LOD_TOTAL_CELLS; ++i) {
        if(0 == (raw[i].coverage_flags & CHUNK_LOD_COVERAGE_MASK)) {
            continue;
        }

        auto rgb = lod::to_rgb8(raw[i].color);
        std::uint32_t key = 0;
        key |= static_cast<std::uint32_t>(rgb[0]) << 16U;
        key |= static_cast<std::uint32_t>(rgb[1]) << 8U;
        key |= static_cast<std::uint32_t>(rgb[2]);

        if(unique.insert(key).second && unique.size() > lod::PALETTE_MAX) {
            overflow = true;
            break;
        }
    }

    lod.color_mode = overflow ? lod::COLOR_RGB332 : lod::COLOR_PALETTE8;
    lod.palette.clear();

    vx::hash_map<std::uint32_t, std::uint8_t> indices;

    for(std::uint8_t level = CHUNK_LOD_LEVEL_MIN; level <= CHUNK_LOD_LEVEL_MAX; level += 1) {
        auto cells = lod.cells(level);
        auto offset = LEVEL_OFFSETS[level];

        for(std::size_t i = 0; i < cells.size(); ++i) {
            const auto& source = raw[offset + i];
            auto& dest = cells[i];

            dest.coverage_flags = source.coverage_flags;

            if(0 == (source.coverage_flags & CHUNK_LOD_COVERAGE_MASK)) {
                dest.color = 0;
                continue;
            }

            if(overflow) {
                dest.color = lod::to_rgb332(source.color);
                continue;
            }

            auto rgb = lod::to_rgb8(source.color);
            std::uint32_t key = 0;
            key |= static_cast<std::uint32_t>(rgb[0]) << 16U;
            key |= static_cast<std::uint32_t>(rgb[1]) << 8U;
            key |= static_cast<std::uint32_t>(rgb[2]);

            auto it = indices.find(key);
            std::uint8_t color = 0;

            if(it == indices.end()) {
                color = static_cast<std::uint8_t>(lod.palette.size());
                lod.palette.push_back(rgb);
                indices[key] = color;
            }
            else {
                color = it->second;
            }

            dest.color = color;
        }
    }

    return true;
}

bool chunk_lod::build(const BlockStorage& blocks, std::span<const biome_id_type> biomes, ChunkLod& lod)
{
    thread_local std::array<RawCell, CHUNK_LOD_TOTAL_CELLS> raw;

    std::fill(raw.begin(), raw.end(), RawCell { Eigen::Vector3f::Zero(), 0 });

    build_level_1(blocks, biomes, raw);
    build_levels(raw);

    return pack_lod(raw, lod);
}

static vx::hash_set<ChunkPos> s_dirty;

static void on_block_update(const BlockUpdateEvent& event)
{
    auto chunk = world::find_chunk(event.cpos());

    if(chunk == nullptr) {
        return;
    }

    // Only a chunk that already streams a pyramid has anything to rebuild;
    // one that never had solid geometry gets its pyramid when it matters.
    if(!world::chunk_registry.all_of<ChunkLod_Component>(chunk->entity())) {
        return;
    }

    s_dirty.insert(event.cpos());
}

void chunk_lod::init(void)
{
    s_dirty.clear();
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}

void chunk_lod::shutdown(void)
{
    s_dirty.clear();
}

void chunk_lod::fixed_update(void)
{
    auto budget = MAX_REBUILDS_PER_TICK;

    while(budget > 0 && !s_dirty.empty()) {
        auto it = s_dirty.begin();
        auto pos = *it;
        s_dirty.erase(it);
        budget -= 1;

        auto chunk = world::find_chunk(pos);

        if(chunk == nullptr) {
            continue;
        }

        auto entity = chunk->entity();

        if(!world::chunk_registry.all_of<ChunkLod_Component>(entity)) {
            continue;
        }

        auto biomes = std::span<const biome_id_type>();

        if(world::chunk_registry.all_of<BiomeSlice_Component>(entity)) {
            auto& slice = world::chunk_registry.get<BiomeSlice_Component>(entity);
            biomes = std::span<const biome_id_type>(*slice.ptr);
        }

        // An emptied chunk keeps an empty pyramid on purpose: the clients in
        // range have to be told to drop the geometry they still hold.
        auto lod = std::make_shared<ChunkLod>();
        chunk_lod::build(chunk->blocks(), biomes, *lod);
        world::chunk_registry.emplace_or_replace<ChunkLod_Component>(entity, ChunkLod_Component { std::move(lod) });

        globals::dispatcher.trigger(ChunkLodUpdatedEvent(pos));
    }
}

ChunkLodUpdatedEvent::ChunkLodUpdatedEvent(const ChunkPos& pos) : m_pos(pos)
{
    // empty
}
