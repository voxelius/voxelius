#ifndef B2F5B0C7_0B1D_4E6C_9B8A_1D3F5E7A9C01
#define B2F5B0C7_0B1D_4E6C_9B8A_1D3F5E7A9C01

#include "shared/coord.hh"
#include "shared/entity/component.hh"
#include "shared/world/block.hh"

class World;

namespace physics
{
struct Body final {
    Eigen::AlignedBox3f local_box;
    float step_height;
};
} // namespace physics

namespace physics
{
struct Contact final {
    block_id_type block { BLOCK_ID_NULL };
    entt::entity entity { entt::null };
};
} // namespace physics

namespace physics
{
struct MoveResult final {
    Eigen::Vector3f position;
    Eigen::Vector3f movement;
    std::array<Contact, 3> axis_contacts;
    Contact support;
    bool on_ground { false };
};
} // namespace physics

namespace physics
{
struct Fluid final {
    block_id_type block { BLOCK_ID_NULL };
    fluid_id_type fluid { FLUID_ID_NULL };
    float volume_fraction { 0.0f };
    float submersion { 0.0f };
    Eigen::Vector3f flow { Eigen::Vector3f::Zero() };
};
} // namespace physics

namespace physics
{
Eigen::AlignedBox3f bounds(const Eigen::Vector3f& position, const Eigen::AlignedBox3f& local_box);
} // namespace physics

namespace physics
{
Fluid sample_fluid(World* world, const ChunkPos& chunk, const Eigen::Vector3f& position, const Eigen::AlignedBox3f& local_box);
} // namespace physics

namespace physics
{
// The caller passes was_on_ground from the previous tick, because a body steps
// up only while it rests on the ground. Positions use the given chunk's frame.
MoveResult move(World* world, const ChunkPos& chunk, const Eigen::Vector3f& position, const Eigen::Vector3f& movement, const Body& body,
    bool was_on_ground);
} // namespace physics

#endif /* B2F5B0C7_0B1D_4E6C_9B8A_1D3F5E7A9C01 */
