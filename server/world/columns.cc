#include "server/pch.hh"

#include "server/world/columns.hh"

#include "core/buffer.hh"
#include "core/utils/modulo.hh"

#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

#include "server/constant.hh"
#include "server/utils/heightmap.hh"
#include "server/world/world.hh"
#include "server/world/zcol_file.hh"

constexpr static std::size_t COLUMN_BYTES = 2 * sizeof(std::int32_t);

static ChunkPosXZ region_of_xz(const ChunkPosXZ& cpos)
{
    return ChunkPosXZ(cpos.x() >> constant::REGION_SIZE_LOG2, cpos.y() >> constant::REGION_SIZE_LOG2);
}

static std::filesystem::path region_path(const ServerWorld* world, const ChunkPosXZ& region)
{
    auto ux = std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(region.x()));
    auto uz = std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(region.y()));
    auto name = std::format("R{:08X}{:08X}.zcol", ux, uz);
    return world->root() / "slice" / name;
}

static std::size_t slot_of(const ChunkPosXZ& cpos)
{
    auto region = region_of_xz(cpos);
    auto lx = static_cast<std::size_t>(cpos.x() - (region.x() << constant::REGION_SIZE_LOG2));
    auto lz = static_cast<std::size_t>(cpos.y() - (region.y() << constant::REGION_SIZE_LOG2));
    return lz * constant::REGION_SIZE + lx;
}

static bool is_surface(World* world, const BlockPos& pos)
{
    auto definition = block_registry::find_definition(world->get_block(pos));
    return definition && !definition->replaceable && !definition->fluid;
}

static bool is_liquid(World* world, const BlockPos& pos)
{
    auto definition = block_registry::find_definition(world->get_block(pos));
    return definition && definition->fluid;
}

static BlockPos::value_type find_surface(World* world, const BlockPos& origin, BlockPos::value_type from)
{
    auto& definition = world->definition();

    for(auto y = from; y >= definition.min_y; --y) {
        if(is_surface(world, BlockPos(origin.x(), y, origin.z()))) {
            return y;
        }
    }

    return Column::UNSET;
}

static BlockPos::value_type find_liquid(World* world, const BlockPos& origin, BlockPos::value_type surface, BlockPos::value_type from)
{
    auto& definition = world->definition();

    for(auto y = from; y > surface && y >= definition.min_y; --y) {
        if(is_liquid(world, BlockPos(origin.x(), y, origin.z()))) {
            return y;
        }
    }

    return Column::UNSET;
}

static void recompute(World* world, const BlockPos& bpos, Column& column)
{
    auto y = bpos.y();

    if(is_surface(world, bpos)) {
        if(column.surface_y == Column::UNSET || y > column.surface_y) {
            column.surface_y = y;
        }
    }
    else if(y == column.surface_y) {
        column.surface_y = find_surface(world, bpos, y - 1);
    }

    if(is_liquid(world, bpos)) {
        auto above_surface = column.surface_y == Column::UNSET || y > column.surface_y;

        if(above_surface && (column.liquid_y == Column::UNSET || y > column.liquid_y)) {
            column.liquid_y = y;
        }
    }
    else if(y == column.liquid_y) {
        column.liquid_y = find_liquid(world, bpos, column.surface_y, y - 1);
    }

    if(column.liquid_y != Column::UNSET && column.liquid_y <= column.surface_y) {
        column.liquid_y = find_liquid(world, bpos, column.surface_y, column.surface_y + 1);
    }
}

static void on_block_update(const BlockUpdateEvent& event)
{
    auto world = static_cast<ServerWorld*>(event.world());
    world->columns().touch(event.bpos());
}

ColumnUpdateEvent::ColumnUpdateEvent(World* world, const BlockPosXZ& pos, const Column& column)
    : m_world(world), m_pos(pos), m_column(column)
{
    assert(world);
}

void WorldColumns::init(ServerWorld* world)
{
    m_world = world;

    std::error_code ec;
    std::filesystem::create_directories(world->root() / "slice", ec);
}

ZcolFile& WorldColumns::region_file(const ChunkPosXZ& cpos)
{
    auto region = region_of_xz(cpos);

    auto it = m_regions.find(region);

    if(it != m_regions.cend()) {
        return *it->second;
    }

    auto file = std::make_unique<ZcolFile>(region_path(m_world, region));
    auto& entry = *file;
    m_regions.emplace(region, std::move(file));
    return entry;
}

bool WorldColumns::read(const ChunkPosXZ& cpos, Chunk& chunk)
{
    auto& file = region_file(cpos);

    if(!file.is_valid()) {
        return false;
    }

    ReadBuffer buffer;

    if(!file.read_slot(slot_of(cpos), buffer)) {
        return false;
    }

    // The payload carries no header of its own, so a short slot has to be
    // rejected here: reading past its end would hand back zeros as heights
    if(!buffer.has_records(static_cast<std::uint32_t>(constant::CHUNK_AREA), COLUMN_BYTES)) {
        return false;
    }

    for(auto& column : chunk.columns) {
        Column::decode(column, buffer);
    }

    BiomeSlice::decode(chunk.biomes, buffer);
    return true;
}

void WorldColumns::write(const ChunkPosXZ& cpos, const Chunk& chunk)
{
    auto& file = region_file(cpos);

    if(!file.is_valid()) {
        return;
    }

    WriteBuffer buffer;

    for(const auto& column : chunk.columns) {
        Column::encode(column, buffer);
    }

    BiomeSlice::encode(chunk.biomes, buffer);
    file.write_slot(slot_of(cpos), buffer);
}

WorldColumns::Chunk& WorldColumns::ensure(const ChunkPosXZ& cpos)
{
    ZoneScopedN("columns::ensure");

    auto it = m_chunks.find(cpos);

    if(it != m_chunks.cend()) {
        return it->second;
    }

    auto& chunk = m_chunks[cpos];

    if(!read(cpos, chunk)) {
        // The probe must not run on the main thread; the load and the generation tasks fill the map first
        chunk.columns = utils::heightmap_probe(m_world, cpos);
        chunk.biomes = m_world->biomes(cpos);
        chunk.dirty = true;
    }

    return chunk;
}

const WorldColumns::Chunk& WorldColumns::get(const ChunkPosXZ& cpos)
{
    return ensure(cpos);
}

const Column* WorldColumns::find(const BlockPosXZ& bpos) const
{
    auto it = m_chunks.find(utils::to_chunk_xz(bpos));

    if(it == m_chunks.cend()) {
        return nullptr;
    }

    auto lpos = utils::to_local_xz(bpos);
    auto index = utils::to_index_xz(lpos);
    return &it->second.columns[index];
}

void WorldColumns::touch(const BlockPos& bpos)
{
    auto cpos = ChunkPosXZ(bpos.x() >> constant::CHUNK_SIZE_LOG2, bpos.z() >> constant::CHUNK_SIZE_LOG2);
    auto lx = static_cast<std::size_t>(utils::mod_signed<BlockPos::value_type>(bpos.x(), constant::CHUNK_SIZE));
    auto lz = static_cast<std::size_t>(utils::mod_signed<BlockPos::value_type>(bpos.z(), constant::CHUNK_SIZE));
    auto index = lx + lz * constant::CHUNK_SIZE;

    auto& chunk = ensure(cpos);
    auto& old = chunk.columns[index];

    auto column = old;
    recompute(m_world, bpos, column);

    if(old.surface_y == column.surface_y && old.liquid_y == column.liquid_y) {
        return;
    }

    old = column;
    chunk.dirty = true;

    m_dirty.insert(cpos);
    m_updates.insert_or_assign(BlockPosXZ(bpos.x(), bpos.z()), std::move(column));
}

void WorldColumns::flush(void)
{
    for(const auto& [pos, column] : m_updates) {
        globals::dispatcher.trigger(ColumnUpdateEvent(m_world, pos, column));
    }

    m_updates.clear();

    for(const auto& cpos : m_dirty) {
        auto it = m_chunks.find(cpos);

        if(it == m_chunks.cend()) {
            continue;
        }

        write(cpos, it->second);
        it->second.dirty = false;
    }

    m_dirty.clear();
}

void WorldColumns::save(void)
{
    flush();

    for(auto& [cpos, chunk] : m_chunks) {
        if(!chunk.dirty) {
            continue;
        }

        write(cpos, chunk);
        chunk.dirty = false;
    }
}

void columns::init(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}

void columns::shutdown(void)
{
    globals::dispatcher.sink<BlockUpdateEvent>().disconnect<&on_block_update>();
}
