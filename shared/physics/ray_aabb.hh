#ifndef DD1519F9_BFB2_4D8F_A405_C48ECC5EEF9D
#define DD1519F9_BFB2_4D8F_A405_C48ECC5EEF9D

class RayAABB final {
public:
    struct Hit final {
        Eigen::Vector3f point;
        Eigen::Vector3f normal;
        float distance;
    };

    RayAABB(void) = default;

    explicit RayAABB(const Eigen::Vector3f& start, const Eigen::Vector3f& direction);

    void reset(const Eigen::Vector3f& start, const Eigen::Vector3f& direction);

    std::optional<Hit> intersect(const Eigen::AlignedBox3f& aabb) const;

    constexpr const Eigen::Vector3f& start(void) const;
    constexpr const Eigen::Vector3f& direction(void) const;
    constexpr const Eigen::Vector3f& inverse_direction(void) const;

private:
    Eigen::Vector3f m_start;
    Eigen::Vector3f m_direction;
    Eigen::Vector3f m_inverse_direction;
};

constexpr const Eigen::Vector3f& RayAABB::start(void) const
{
    return m_start;
}

constexpr const Eigen::Vector3f& RayAABB::direction(void) const
{
    return m_direction;
}

constexpr const Eigen::Vector3f& RayAABB::inverse_direction(void) const
{
    return m_inverse_direction;
}

#endif /* DD1519F9_BFB2_4D8F_A405_C48ECC5EEF9D */
