#include "core/pch.hh"

#include "core/camera.hh"

Eigen::Vector3f Camera::position(void) const
{
    return -m_view.block<3, 3>(0, 0).transpose() * m_view.block<3, 1>(0, 3);
}

Eigen::Vector3f Camera::forward_vector(void) const
{
    return -m_view.row(2).head<3>();
}

Eigen::Vector3f Camera::right_vector(void) const
{
    return m_view.row(0).head<3>();
}

Eigen::Vector3f Camera::up_vector(void) const
{
    return m_view.row(1).head<3>();
}

Eigen::Quaternionf Camera::orientation(void) const
{
    Eigen::Matrix3f rotation;
    rotation.col(0) = right_vector();
    rotation.col(1) = up_vector();
    rotation.col(2) = -forward_vector();

    return Eigen::Quaternionf(rotation).normalized();
}

Eigen::Vector3f Camera::angles(void) const
{
    Eigen::Matrix3f rotation;
    rotation.col(0) = right_vector();
    rotation.col(1) = up_vector();
    rotation.col(2) = -forward_vector();

    return rotation.canonicalEulerAngles(0, 1, 2); // Pitch, yaw, roll
}

void Camera::set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far)
{
    assert(std::isfinite(left));
    assert(std::isfinite(right));
    assert(std::isfinite(bottom));
    assert(std::isfinite(top));
    assert(std::isfinite(z_near));
    assert(std::isfinite(z_far));
    assert(z_near < z_far);

    m_projection.setIdentity();
    m_projection(0, 0) = 2.0f / (right - left);
    m_projection(1, 1) = 2.0f / (top - bottom);
    m_projection(2, 2) = -1.0f / (z_far - z_near);
    m_projection(0, 3) = -1.0f * (right + left) / (right - left);
    m_projection(1, 3) = -1.0f * (top + bottom) / (top - bottom);
    m_projection(2, 3) = -z_near / (z_far - z_near);

    m_view_projection.setConstant(std::numeric_limits<float>::quiet_NaN());
}

void Camera::set_perspective(float fov_y, float aspect_ratio, float z_near, float z_far)
{
    assert(std::isfinite(fov_y));
    assert(std::isfinite(aspect_ratio));
    assert(std::isfinite(z_near));
    assert(std::isfinite(z_far));
    assert(z_near < z_far);

    auto inv_tangent = 1.0f / std::tan(0.5f * fov_y);

    m_projection.setZero();
    m_projection(0, 0) = inv_tangent / aspect_ratio;
    m_projection(1, 1) = inv_tangent;
    m_projection(2, 2) = -z_far / (z_far - z_near);
    m_projection(3, 2) = -1.0f;
    m_projection(2, 3) = -z_far * z_near / (z_far - z_near);

    m_view_projection.setConstant(std::numeric_limits<float>::quiet_NaN());
}

void Camera::set_view(const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation)
{
    assert(position.allFinite());
    assert(orientation.coeffs().allFinite());

    Eigen::Vector3f right((orientation * Eigen::Vector3f::UnitX()).normalized());
    Eigen::Vector3f up((orientation * Eigen::Vector3f::UnitY()).normalized());
    Eigen::Vector3f forward((orientation * -Eigen::Vector3f::UnitZ()).normalized());

    m_view.setIdentity();

    m_view.row(0).head<3>() = right;
    m_view.row(1).head<3>() = up;
    m_view.row(2).head<3>() = -forward;

    m_view(0, 3) = -right.dot(position);
    m_view(1, 3) = -up.dot(position);
    m_view(2, 3) = forward.dot(position);

    m_view_projection.setConstant(std::numeric_limits<float>::quiet_NaN());
}

void Camera::set_view(const Eigen::Vector3f& position, const Eigen::Vector3f& angles)
{
    assert(position.allFinite());
    assert(angles.allFinite());

    Eigen::AngleAxisf pitch(angles.x(), Eigen::Vector3f::UnitX());
    Eigen::AngleAxisf yaw(angles.y(), Eigen::Vector3f::UnitY());
    Eigen::AngleAxisf roll(angles.z(), Eigen::Vector3f::UnitZ());

    set_view(position, Eigen::Quaternionf(yaw * pitch * roll));
}

void Camera::set_look(const Eigen::Vector3f& position, const Eigen::Vector3f& target)
{
    set_look(position, target, Eigen::Vector3f::UnitY());
}

void Camera::set_look(const Eigen::Vector3f& position, const Eigen::Vector3f& target, const Eigen::Vector3f& up)
{
    assert(position.allFinite());
    assert(target.allFinite());
    assert(up.allFinite());

    Eigen::Vector3f forward((target - position).normalized());
    Eigen::Vector3f right(forward.cross(up).normalized());
    Eigen::Vector3f up_corrected(right.cross(forward).normalized());

    m_view.setIdentity();

    m_view(0, 0) = right.x();
    m_view(0, 1) = right.y();
    m_view(0, 2) = right.z();
    m_view(0, 3) = -right.dot(position);

    m_view(1, 0) = up_corrected.x();
    m_view(1, 1) = up_corrected.y();
    m_view(1, 2) = up_corrected.z();
    m_view(1, 3) = -up_corrected.dot(position);

    m_view(2, 0) = -forward.x();
    m_view(2, 1) = -forward.y();
    m_view(2, 2) = -forward.z();
    m_view(2, 3) = forward.dot(position);

    m_view_projection.setConstant(std::numeric_limits<float>::quiet_NaN());
}

void Camera::update(void)
{
    if(m_view_projection.hasNaN()) {
        m_view_projection = m_projection * m_view;
        m_frustum.set_matrix(m_view_projection);
    }
}
