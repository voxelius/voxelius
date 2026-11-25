#ifndef AB9A226A_0868_4978_BF1B_14ED1B503A24
#define AB9A226A_0868_4978_BF1B_14ED1B503A24

#include "shared/coord.hh"
#include "shared/world/block.hh"

class Chunk;
class World;

class RayDDA final {
public:
    RayDDA(void) = default;
    RayDDA(World* world, const ChunkPos& start_chunk, const Eigen::Vector3f& start_pos, const Eigen::Vector3f& direction);

    void reset(const ChunkPos& start_chunk, const Eigen::Vector3f& start_pos, const Eigen::Vector3f& direction);

    block_id_type step(void);

    constexpr const ChunkPos& start_chunk(void) const;
    constexpr const Eigen::Vector3f& start_pos(void) const;
    constexpr const Eigen::Vector3f& direction(void) const;

    constexpr const Eigen::Vector3f& delta_distance(void) const;
    constexpr const Eigen::Vector3f& side_distance(void) const;
    constexpr const BlockPos& block_step(void) const;

    constexpr float distance(void) const;
    constexpr const BlockPos& position(void) const;
    constexpr const BlockPos& normal(void) const;

private:
    World* m_world { nullptr };
    ChunkPos m_start_chunk;
    Eigen::Vector3f m_start_pos;
    Eigen::Vector3f m_direction;

    Eigen::Vector3f m_delta_distance;
    Eigen::Vector3f m_side_distance;
    BlockPos m_block_step;

    float m_distance;
    BlockPos m_position;
    BlockPos m_normal;

    ChunkPos m_last_chunk_pos;
    std::shared_ptr<Chunk> m_last_chunk;
};

constexpr const ChunkPos& RayDDA::start_chunk(void) const
{
    return m_start_chunk;
}

constexpr const Eigen::Vector3f& RayDDA::start_pos(void) const
{
    return m_start_pos;
}

constexpr const Eigen::Vector3f& RayDDA::direction(void) const
{
    return m_direction;
}

constexpr const Eigen::Vector3f& RayDDA::delta_distance(void) const
{
    return m_delta_distance;
}

constexpr const Eigen::Vector3f& RayDDA::side_distance(void) const
{
    return m_side_distance;
}

constexpr const BlockPos& RayDDA::block_step(void) const
{
    return m_block_step;
}

constexpr float RayDDA::distance(void) const
{
    return m_distance;
}

constexpr const BlockPos& RayDDA::position(void) const
{
    return m_position;
}

constexpr const BlockPos& RayDDA::normal(void) const
{
    return m_normal;
}

#endif /* AB9A226A_0868_4978_BF1B_14ED1B503A24 */
