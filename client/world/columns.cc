#include "client/pch.hh"

#include "client/world/columns.hh"

#include "core/utils/modulo.hh"

#include "shared/constant.hh"

static vx::hash_map<ChunkPosXZ, ColumnSlice> s_chunks;

static std::size_t local_index(BlockPosXZ::value_type value)
{
    return static_cast<std::size_t>(utils::mod_signed<BlockPosXZ::value_type>(value, constant::CHUNK_SIZE));
}

void columns::clear(void)
{
    s_chunks.clear();
}

void columns::prune(const ChunkAlignedBoxXZ& box)
{
    auto it = s_chunks.begin();

    while(it != s_chunks.end()) {
        if(box.contains(it->first)) {
            it = std::next(it);
        }
        else {
            it = s_chunks.erase(it);
        }
    }
}

const Column* columns::find(const BlockPosXZ& bpos)
{
    auto cpos = ChunkPosXZ(bpos.x() >> constant::CHUNK_SIZE_LOG2, bpos.y() >> constant::CHUNK_SIZE_LOG2);
    auto it = s_chunks.find(cpos);

    if(it == s_chunks.cend()) {
        return nullptr;
    }

    auto index = local_index(bpos.x()) + local_index(bpos.y()) * constant::CHUNK_SIZE;
    return &it->second[index];
}

void columns::set(const ChunkPosXZ& cpos, const ColumnSlice& slice)
{
    s_chunks[cpos] = slice;
}

void columns::update(const BlockPosXZ& bpos, const Column& column)
{
    auto cpos = ChunkPosXZ(bpos.x() >> constant::CHUNK_SIZE_LOG2, bpos.y() >> constant::CHUNK_SIZE_LOG2);
    auto it = s_chunks.find(cpos);

    if(it == s_chunks.cend()) {
        return;
    }

    auto index = local_index(bpos.x()) + local_index(bpos.y()) * constant::CHUNK_SIZE;
    it->second[index] = column;
}
