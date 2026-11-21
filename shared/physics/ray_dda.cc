#include "shared/pch.hh"

#include "shared/physics/ray_dda.hh"

#include "shared/utils/coord.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

Ray_DDA::Ray_DDA(World* world, const ChunkPos& start_chunk, const Eigen::Vector3f& start_pos, const Eigen::Vector3f& direction)
    : m_world(world)
{
    reset(start_chunk, start_pos, direction);
}

void Ray_DDA::reset(const ChunkPos& start_chunk, const Eigen::Vector3f& start_pos, const Eigen::Vector3f& direction)
{
    assert(m_world);

    m_start_chunk = start_chunk;
    m_start_pos = start_pos;
    m_direction = direction.normalized();

    auto far_distance = std::numeric_limits<float>::max();

    if(m_direction.x()) {
        m_delta_distance.x() = std::abs(1.0f / m_direction.x());
    }
    else {
        m_delta_distance.x() = far_distance;
    }

    if(m_direction.y()) {
        m_delta_distance.y() = std::abs(1.0f / m_direction.y());
    }
    else {
        m_delta_distance.y() = far_distance;
    }

    if(m_direction.z()) {
        m_delta_distance.z() = std::abs(1.0f / m_direction.z());
    }
    else {
        m_delta_distance.z() = far_distance;
    }

    m_distance = 0.0f;
    m_position = utils::to_block(m_start_chunk, start_pos.cast<LocalPos::value_type>());
    m_normal = BlockPos::Zero();

    auto local_pos = m_start_pos.cast<LocalPos::value_type>();

    if(direction.x() < 0.0f) {
        m_side_distance.x() = m_delta_distance.x() * (m_start_pos.x() - local_pos.x());
        m_block_step.x() = -1;
    }
    else {
        m_side_distance.x() = m_delta_distance.x() * (local_pos.x() + 1.0f - m_start_pos.x());
        m_block_step.x() = 1;
    }

    if(direction.y() < 0.0f) {
        m_side_distance.y() = m_delta_distance.y() * (m_start_pos.y() - local_pos.y());
        m_block_step.y() = -1;
    }
    else {
        m_side_distance.y() = m_delta_distance.y() * (local_pos.y() + 1.0f - m_start_pos.y());
        m_block_step.y() = 1;
    }

    if(direction.z() < 0.0f) {
        m_side_distance.z() = m_delta_distance.z() * (m_start_pos.z() - local_pos.z());
        m_block_step.z() = -1;
    }
    else {
        m_side_distance.z() = m_delta_distance.z() * (local_pos.z() + 1.0f - m_start_pos.z());
        m_block_step.z() = 1;
    }

    m_last_chunk_pos = m_start_chunk;
    m_last_chunk = m_world->find_chunk(m_last_chunk_pos);
}

block_id_type Ray_DDA::step(void)
{
    if(m_side_distance.x() < m_side_distance.z()) {
        if(m_side_distance.x() < m_side_distance.y()) {
            m_normal = BlockPos(-m_block_step.x(), 0, 0);
            m_distance = m_side_distance.x();
            m_side_distance.x() += m_delta_distance.x();
            m_position.x() += m_block_step.x();
        }
        else {
            m_normal = BlockPos(0, -m_block_step.y(), 0);
            m_distance = m_side_distance.y();
            m_side_distance.y() += m_delta_distance.y();
            m_position.y() += m_block_step.y();
        }
    }
    else {
        if(m_side_distance.z() < m_side_distance.y()) {
            m_normal = BlockPos(0, 0, -m_block_step.z());
            m_distance = m_side_distance.z();
            m_side_distance.z() += m_delta_distance.z();
            m_position.z() += m_block_step.z();
        }
        else {
            m_normal = BlockPos(0, -m_block_step.y(), 0);
            m_distance = m_side_distance.y();
            m_side_distance.y() += m_delta_distance.y();
            m_position.y() += m_block_step.y();
        }
    }

    auto new_chunk_pos = utils::to_chunk(m_position);
    auto local_pos = utils::to_local(m_position);

    if(m_last_chunk == nullptr || !m_last_chunk_pos.isApprox(new_chunk_pos)) {
        m_last_chunk_pos = new_chunk_pos;
        m_last_chunk = m_world->find_chunk(m_last_chunk_pos);
    }

    if(m_last_chunk == nullptr) {
        return BLOCK_ID_NULL;
    }

    return m_last_chunk->get_block(local_pos);
}
