#include "shared/pch.hh"

#include "shared/utils/view.hh"

ChunkAlignedBox utils::view_box(const ChunkPos& center, ChunkPos::value_type radius)
{
    auto min = center - ChunkPos::Constant(radius);
    auto max = center + ChunkPos::Constant(radius);
    return ChunkAlignedBox(min, max);
}

ChunkAlignedBoxXZ utils::interest_box(const ChunkPos& center, ChunkPos::value_type radius)
{
    auto min = ChunkPosXZ(center.x() - radius, center.z() - radius);
    auto max = ChunkPosXZ(center.x() + radius, center.z() + radius);
    return ChunkAlignedBoxXZ(min, max);
}
