#ifndef A1B3C5D7_E9F2_4A6C_8B0D_2E4F6A8C0B91
#define A1B3C5D7_E9F2_4A6C_8B0D_2E4F6A8C0B91

#include "shared/world/block.hh"

namespace utils
{
Eigen::Vector3i rotate_offset(const Eigen::Vector3i& offset, block_face facing);
Eigen::AlignedBox3i rotate_bounds(const Eigen::AlignedBox3i& bounds, block_face facing);
} // namespace utils

#endif /* A1B3C5D7_E9F2_4A6C_8B0D_2E4F6A8C0B91 */
