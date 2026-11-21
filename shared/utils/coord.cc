#include "shared/pch.hh"

#include "shared/utils/coord.hh"

#include "core/utils/modulo.hh"

#include "shared/constant.hh"

ChunkPos utils::to_chunk(const BlockPos& bpos)
{
    ChunkPos cpos;
    cpos.x() = static_cast<ChunkPos::value_type>(bpos.x() >> constant::CHUNK_SIZE_LOG2);
    cpos.y() = static_cast<ChunkPos::value_type>(bpos.y() >> constant::CHUNK_SIZE_LOG2);
    cpos.z() = static_cast<ChunkPos::value_type>(bpos.z() >> constant::CHUNK_SIZE_LOG2);
    return cpos;
}

ChunkPosXZ utils::to_chunk_xz(const BlockPosXZ& bpos)
{
    ChunkPosXZ cpos;
    cpos[0] = static_cast<ChunkPosXZ::value_type>(bpos[0] >> constant::CHUNK_SIZE_LOG2);
    cpos[1] = static_cast<ChunkPosXZ::value_type>(bpos[1] >> constant::CHUNK_SIZE_LOG2);
    return cpos;
}

LocalPos utils::to_local(const BlockPos& bpos)
{
    LocalPos lpos;
    lpos.x() = static_cast<LocalPos::value_type>(utils::mod_signed<BlockPos::value_type>(bpos.x(), constant::CHUNK_SIZE));
    lpos.y() = static_cast<LocalPos::value_type>(utils::mod_signed<BlockPos::value_type>(bpos.y(), constant::CHUNK_SIZE));
    lpos.z() = static_cast<LocalPos::value_type>(utils::mod_signed<BlockPos::value_type>(bpos.z(), constant::CHUNK_SIZE));
    return lpos;
}

LocalPos utils::to_local(std::size_t index)
{
    LocalPos lpos;
    auto plane = index / constant::CHUNK_SIZE;
    lpos.x() = static_cast<LocalPos::value_type>(index % constant::CHUNK_SIZE);
    lpos.y() = static_cast<LocalPos::value_type>(plane / constant::CHUNK_SIZE);
    lpos.z() = static_cast<LocalPos::value_type>(plane % constant::CHUNK_SIZE);
    return lpos;
}

LocalPosXZ utils::to_local_xz(const BlockPosXZ& bpos)
{
    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(bpos[0], constant::CHUNK_SIZE));
    lpos[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<BlockPosXZ::value_type>(bpos[1], constant::CHUNK_SIZE));
    return lpos;
}

LocalPosXZ utils::to_local_xz(std::size_t index)
{
    LocalPosXZ lpos;
    lpos[0] = static_cast<LocalPosXZ::value_type>(index % constant::CHUNK_SIZE);
    lpos[1] = static_cast<LocalPosXZ::value_type>(index / constant::CHUNK_SIZE);
    return lpos;
}

BlockPos utils::to_block(const ChunkPos& cpos)
{
    BlockPos bpos;
    bpos.x() = static_cast<BlockPos::value_type>(cpos.x() << constant::CHUNK_SIZE_LOG2);
    bpos.y() = static_cast<BlockPos::value_type>(cpos.y() << constant::CHUNK_SIZE_LOG2);
    bpos.z() = static_cast<BlockPos::value_type>(cpos.z() << constant::CHUNK_SIZE_LOG2);
    return bpos;
}

BlockPos utils::to_block(const ChunkPos& cpos, const LocalPos& lpos)
{
    BlockPos bpos;
    bpos.x() = static_cast<BlockPos::value_type>(lpos.x()) + (static_cast<BlockPos::value_type>(cpos.x()) << constant::CHUNK_SIZE_LOG2);
    bpos.y() = static_cast<BlockPos::value_type>(lpos.y()) + (static_cast<BlockPos::value_type>(cpos.y()) << constant::CHUNK_SIZE_LOG2);
    bpos.z() = static_cast<BlockPos::value_type>(lpos.z()) + (static_cast<BlockPos::value_type>(cpos.z()) << constant::CHUNK_SIZE_LOG2);
    return bpos;
}

BlockPosXZ utils::to_block_xz(const ChunkPosXZ& cpos)
{
    BlockPosXZ bpos;
    bpos[0] = static_cast<BlockPosXZ::value_type>(cpos[0] << constant::CHUNK_SIZE_LOG2);
    bpos[1] = static_cast<BlockPosXZ::value_type>(cpos[1] << constant::CHUNK_SIZE_LOG2);
    return bpos;
}

BlockPosXZ utils::to_block_xz(const ChunkPosXZ& cpos, const LocalPosXZ& lpos)
{
    BlockPosXZ bpos;
    bpos[0] = static_cast<BlockPosXZ::value_type>(lpos[0]) + (static_cast<BlockPosXZ::value_type>(cpos[0]) << constant::CHUNK_SIZE_LOG2);
    bpos[1] = static_cast<BlockPosXZ::value_type>(lpos[1]) + (static_cast<BlockPosXZ::value_type>(cpos[1]) << constant::CHUNK_SIZE_LOG2);
    return bpos;
}

std::size_t utils::to_index(const LocalPos& lpos)
{
    return static_cast<std::size_t>((lpos.y() * constant::CHUNK_SIZE + lpos.z()) * constant::CHUNK_SIZE + lpos.x());
}

std::size_t utils::to_index_xz(const LocalPosXZ& lpos)
{
    return static_cast<std::size_t>(lpos[1] * constant::CHUNK_SIZE + lpos[0]);
}

Eigen::Vector3f utils::to_relative(const ChunkPos& pivot_cpos, const ChunkPos& cpos, const Eigen::Vector3f& fvec)
{
    Eigen::Vector3f rpos;
    rpos.x() = static_cast<float>((cpos.x() - pivot_cpos.x()) << constant::CHUNK_SIZE_LOG2) + fvec.x();
    rpos.y() = static_cast<float>((cpos.y() - pivot_cpos.y()) << constant::CHUNK_SIZE_LOG2) + fvec.y();
    rpos.z() = static_cast<float>((cpos.z() - pivot_cpos.z()) << constant::CHUNK_SIZE_LOG2) + fvec.z();
    return rpos;
}

Eigen::Vector3f utils::to_relative(const ChunkPos& pivot_cpos, const Eigen::Vector3f& pivot_fvec, const ChunkPos& cpos)
{
    Eigen::Vector3f rpos;
    rpos.x() = static_cast<float>((cpos.x() - pivot_cpos.x()) << constant::CHUNK_SIZE_LOG2) - pivot_fvec.x();
    rpos.y() = static_cast<float>((cpos.y() - pivot_cpos.y()) << constant::CHUNK_SIZE_LOG2) - pivot_fvec.y();
    rpos.z() = static_cast<float>((cpos.z() - pivot_cpos.z()) << constant::CHUNK_SIZE_LOG2) - pivot_fvec.z();
    return rpos;
}

Eigen::Vector3f utils::to_relative(const ChunkPos& pivot_cpos, const Eigen::Vector3f& pivot_fvec, const ChunkPos& cpos,
    const Eigen::Vector3f& fvec)
{
    Eigen::Vector3f rpos;
    rpos.x() = static_cast<float>((cpos.x() - pivot_cpos.x()) << constant::CHUNK_SIZE_LOG2) + (fvec.x() - pivot_fvec.x());
    rpos.y() = static_cast<float>((cpos.y() - pivot_cpos.y()) << constant::CHUNK_SIZE_LOG2) + (fvec.y() - pivot_fvec.y());
    rpos.z() = static_cast<float>((cpos.z() - pivot_cpos.z()) << constant::CHUNK_SIZE_LOG2) + (fvec.z() - pivot_fvec.z());
    return rpos;
}

Eigen::Vector3f utils::to_fvec(const ChunkPos& cpos)
{
    Eigen::Vector3f rpos;
    rpos.x() = static_cast<float>(cpos.x() << constant::CHUNK_SIZE_LOG2);
    rpos.y() = static_cast<float>(cpos.y() << constant::CHUNK_SIZE_LOG2);
    rpos.z() = static_cast<float>(cpos.z() << constant::CHUNK_SIZE_LOG2);
    return rpos;
}

Eigen::Vector3f utils::to_fvec(const ChunkPos& cpos, const Eigen::Vector3f& offset)
{
    Eigen::Vector3f rpos;
    rpos.x() = offset.x() + static_cast<float>(cpos.x() << constant::CHUNK_SIZE_LOG2);
    rpos.y() = offset.y() + static_cast<float>(cpos.y() << constant::CHUNK_SIZE_LOG2);
    rpos.z() = offset.z() + static_cast<float>(cpos.z() << constant::CHUNK_SIZE_LOG2);
    return rpos;
}

LocalPos utils::wrap_local(const LocalPos& lpos)
{
    LocalPos result;
    result.x() = static_cast<LocalPos::value_type>(utils::mod_signed<LocalPos::value_type>(lpos.x(), constant::CHUNK_SIZE));
    result.y() = static_cast<LocalPos::value_type>(utils::mod_signed<LocalPos::value_type>(lpos.y(), constant::CHUNK_SIZE));
    result.z() = static_cast<LocalPos::value_type>(utils::mod_signed<LocalPos::value_type>(lpos.z(), constant::CHUNK_SIZE));
    return result;
}

LocalPosXZ utils::wrap_local_xz(const LocalPosXZ& lpos)
{
    LocalPosXZ result;
    result[0] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<LocalPosXZ::value_type>(lpos[0], constant::CHUNK_SIZE));
    result[1] = static_cast<LocalPosXZ::value_type>(utils::mod_signed<LocalPosXZ::value_type>(lpos[1], constant::CHUNK_SIZE));
    return result;
}
