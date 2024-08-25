#pragma once
#include <game/shared/voxel.hh>
#include <string>
#include <unordered_map>
#include <vector>

enum class VoxelFace : unsigned int {
    Invalid     = 0xFFFF,

    CubeNorth   = 0x0000,
    CubeSouth   = 0x0001,
    CubeEast    = 0x0002,
    CubeWest    = 0x0003,
    CubeTop     = 0x0004,
    CubeBottom  = 0x0005,
    CubeCount   = 0x0006,

    SlabNorth   = CubeNorth,
    SlabSouth   = CubeSouth,
    SlabEast    = CubeEast,
    SlabWest    = CubeWest,
    SlabTop     = CubeTop,
    SlabBottom  = CubeBottom,
    SlabCount   = CubeCount,

    StairNorth  = CubeNorth,
    StairSouth  = CubeSouth,
    StairEast   = CubeEast,
    StairWest   = CubeWest,
    StairTop    = CubeTop,
    StairBottom = CubeBottom,
    StairCount  = CubeCount,

    CrossNESW   = 0x0000,
    CrossNWSE   = 0x0001,
    CrossCount  = 0x0002,
};

enum class VoxelType : unsigned int {
    Cube        = 0x0000,
    Slab        = 0x0001, // TODO
    Stairs      = 0x0002, // TODO
    Cross       = 0x0003, // TODO
    VModel      = 0x0004, // TODO
    DModel      = 0x0005, // TODO
};

using VoxelFacing = unsigned short;
constexpr static VoxelFacing FACING_NORTH   = 0x0000;
constexpr static VoxelFacing FACING_SOUTH   = 0x0001;
constexpr static VoxelFacing FACING_EAST    = 0x0002;
constexpr static VoxelFacing FACING_WEST    = 0x0003;
constexpr static VoxelFacing FACING_UP      = 0x0004;
constexpr static VoxelFacing FACING_DOWN    = 0x0005;
constexpr static VoxelFacing FACING_NESW    = 0x0006;
constexpr static VoxelFacing FACING_NWSE    = 0x0007;

using VoxelVis = unsigned short;
constexpr static VoxelVis VIS_NORTH = 1 << FACING_NORTH;
constexpr static VoxelVis VIS_SOUTH = 1 << FACING_SOUTH;
constexpr static VoxelVis VIS_EAST  = 1 << FACING_EAST;
constexpr static VoxelVis VIS_WEST  = 1 << FACING_WEST;
constexpr static VoxelVis VIS_UP    = 1 << FACING_UP;
constexpr static VoxelVis VIS_DOWN  = 1 << FACING_DOWN;

struct VoxelTexture final {
    std::vector<std::string> paths {};
    std::size_t cached_offset {};
    std::size_t cached_plane {};
};

struct VoxelInfo final {
    std::vector<VoxelTexture> textures {};
    std::string name {};
    VoxelType type {};
    bool animated {};
    bool blending {};
    Voxel base {};
};

class VDefBuilder final {
public:
    VDefBuilder(void) = delete;
    VDefBuilder(const std::string &name, VoxelType type);
    virtual ~VDefBuilder(void) = default;

public:
    VDefBuilder &add_state(const std::string &name);
    VDefBuilder &add_default_state(void);
    VDefBuilder &add_slab_states(void);
    VDefBuilder &add_stairs_states(void);

public:
    Voxel build(void) const;

private:
    std::vector<std::string> states {};
    std::string name {};
    VoxelType type {};
};

namespace vdef
{
extern std::unordered_map<std::string, VDefBuilder> builders;
extern std::unordered_map<std::string, Voxel> names;
extern std::vector<VoxelInfo> voxels;
} // namespace vdef

namespace vdef
{
VDefBuilder &create(const std::string &name, VoxelType type);
VoxelInfo *find(const std::string &name);
VoxelInfo *find(const Voxel voxel);
} // namespace vdef

namespace vdef
{
void purge(void);
} // namespace vdef
