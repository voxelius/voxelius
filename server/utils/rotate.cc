#include "server/pch.hh"

#include "server/utils/rotate.hh"

static Eigen::Vector3i rotate_vector(const Eigen::Vector3i& vector, block_face facing)
{
    switch(facing) {
        case BLOCK_FACE_NORTH:
            return vector;

        case BLOCK_FACE_SOUTH:
            return Eigen::Vector3i(-vector.x(), vector.y(), -vector.z());

        case BLOCK_FACE_EAST:
            return Eigen::Vector3i(vector.z(), vector.y(), -vector.x());

        case BLOCK_FACE_WEST:
            return Eigen::Vector3i(-vector.z(), vector.y(), vector.x());

        case BLOCK_FACE_TOP:
        case BLOCK_FACE_BOTTOM:
            return vector; // features are only rotated around the vertical axis
    }

    return vector;
}

Eigen::Vector3i utils::rotate_offset(const Eigen::Vector3i& offset, block_face facing)
{
    return rotate_vector(offset, facing);
}

Eigen::AlignedBox3i utils::rotate_bounds(const Eigen::AlignedBox3i& bounds, block_face facing)
{
    if(facing == BLOCK_FACE_NORTH) {
        return bounds;
    }

    Eigen::AlignedBox3i rotated;
    rotated.setEmpty();

    for(Eigen::Index i = 0; i < 8; ++i) {
        auto corner_type = static_cast<Eigen::AlignedBox3i::CornerType>(i);
        auto corner_ident = bounds.corner(corner_type);
        auto corner_rotated = rotate_vector(corner_ident, facing);
        rotated.extend(corner_rotated);
    }

    return rotated;
}
