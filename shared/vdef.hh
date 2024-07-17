// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_VDEF_HH
#define SHARED_VDEF_HH
#include <array>
#include <shared/vfs.hh>
#include <shared/voxel.hh>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using VoxelDraw = unsigned short;
constexpr static VoxelDraw VOXEL_DRAW_NODRAW    = 0xFFFF;
constexpr static VoxelDraw VOXEL_DRAW_OPAQUE    = 0x0000;
constexpr static VoxelDraw VOXEL_DRAW_THRES     = 0x0001;
constexpr static VoxelDraw VOXEL_DRAW_BLEND     = 0x0002;
constexpr static std::size_t NUM_VOXEL_DRAW = 3;

using VoxelFace = unsigned short;
constexpr static VoxelFace VOXEL_FACE_NORTH     = 0x0000;
constexpr static VoxelFace VOXEL_FACE_SOUTH     = 0x0001;
constexpr static VoxelFace VOXEL_FACE_EAST      = 0x0002;
constexpr static VoxelFace VOXEL_FACE_WEST      = 0x0003;
constexpr static VoxelFace VOXEL_FACE_TOP       = 0x0004;
constexpr static VoxelFace VOXEL_FACE_BOTTOM    = 0x0005;
constexpr static VoxelFace VOXEL_FACE_CUSTOM_X  = 0x0006;
constexpr static VoxelFace VOXEL_FACE_CUSTOM_Y  = 0x0007;
constexpr static std::size_t NUM_VOXEL_FACE = 8;

using VoxelType = unsigned short;
constexpr static VoxelType VOXEL_TYPE_CUBE      = 0x0000;
constexpr static VoxelType VOXEL_TYPE_SLAB      = 0x0001;
constexpr static VoxelType VOXEL_TYPE_STAIRS    = 0x0002;
constexpr static VoxelType VOXEL_TYPE_CROSS     = 0x0003;
constexpr static VoxelType VOXEL_TYPE_VMODEL    = 0x0004;
constexpr static VoxelType VOXEL_TYPE_DMODEL    = 0x0005;

struct VoxelTexture final {
    std::vector<vfs::path_t> paths {};
    std::uint16_t cached_offset {};
};

struct VoxelInfo final {
    std::string name {};
    VoxelType type {VOXEL_TYPE_CUBE};
    VoxelDraw draw {VOXEL_DRAW_NODRAW};
    std::array<VoxelTexture, NUM_VOXEL_FACE> textures {};
    Voxel base_voxel {NULL_VOXEL};
};

class VoxelBuilder final {
public:
    VoxelBuilder(void) = delete;
    VoxelBuilder(VoxelType type, const std::string &name);
    virtual ~VoxelBuilder(void) = default;

    VoxelBuilder &state_default(void);
    VoxelBuilder &state(const std::string &name);

    Voxel build(void) const;

private:
    VoxelType type {};
    std::string name {};
    std::unordered_set<std::string> states {};
};

namespace vdef
{
extern std::unordered_map<std::string, VoxelBuilder> builders;
extern std::unordered_set<vfs::path_t, vfs::path_hasher_t> textures;
extern std::vector<VoxelInfo> voxels;
} // namespace vdef

namespace vdef
{
VoxelBuilder &create(VoxelType type, const std::string &name);
VoxelInfo *find(const std::string &name);
VoxelInfo *find(const Voxel voxel);
} // namespace vdef

#endif /* SHARED_VDEF_HH */
