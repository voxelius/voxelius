#include "shared/pch.hh"

#include "shared/physics/ray_aabb.hh"

RayAABB::RayAABB(const Eigen::Vector3f& start, const Eigen::Vector3f& direction)
{
    reset(start, direction);
}

void RayAABB::reset(const Eigen::Vector3f& start, const Eigen::Vector3f& direction)
{
    m_start = start;
    m_direction = direction.normalized();
    m_inverse_direction = m_direction.cwiseInverse();
}

std::optional<RayAABB::Hit> RayAABB::intersect(const Eigen::AlignedBox3f& aabb) const
{
    Eigen::Vector3f t_0 = (aabb.min() - m_start).cwiseProduct(m_inverse_direction);
    Eigen::Vector3f t_1 = (aabb.max() - m_start).cwiseProduct(m_inverse_direction);

    Eigen::Vector3f t_min = t_0.cwiseMin(t_1);
    Eigen::Vector3f t_max = t_0.cwiseMax(t_1);

    auto t_near = t_min.maxCoeff();
    auto t_far = t_max.minCoeff();

    if(t_near > t_far || t_far < 0.0f) {
        return std::nullopt;
    }

    auto t = t_near > 0.0f ? t_near : t_far;

    Hit hit {};
    hit.distance = t;
    hit.point = m_start + m_direction * t;
    hit.normal = Eigen::Vector3f::Zero();

    Eigen::Vector3f p_local = hit.point - aabb.center();
    Eigen::Vector3f half_extents = 0.5f * aabb.sizes();

    Eigen::Vector3f d = p_local.cwiseQuotient(half_extents);
    Eigen::Vector3f d_abs = d.cwiseAbs();

    if(d_abs.x() >= d_abs.y() && d_abs.x() >= d_abs.z()) {
        hit.normal.x() = std::copysign(1.0f, d.x());
    }
    else if(d_abs.y() >= d_abs.z()) {
        hit.normal.y() = std::copysign(1.0f, d.y());
    }
    else {
        hit.normal.z() = std::copysign(1.0f, d.z());
    }

    return hit;
}
