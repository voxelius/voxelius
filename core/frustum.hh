#ifndef CCA9F659_CACE_4327_BC1D_1E658B29B0AB
#define CCA9F659_CACE_4327_BC1D_1E658B29B0AB

class Frustum final {
public:
    constexpr static unsigned PLN_LF = 0;
    constexpr static unsigned PLN_RT = 1;
    constexpr static unsigned PLN_DN = 2;
    constexpr static unsigned PLN_UP = 3;
    constexpr static unsigned PLN_FT = 4;
    constexpr static unsigned PLN_BK = 5;

    explicit Frustum(void);
    explicit Frustum(const Eigen::Matrix4f& matrix);

    constexpr const Eigen::Hyperplane<float, 3>& plane(unsigned index) const;
    void set_plane(unsigned index, Eigen::Hyperplane<float, 3> plane);
    void set_matrix(const Eigen::Matrix4f& matrix);

    bool contains(const Eigen::Vector3f& point) const;
    bool intersects(const Eigen::AlignedBox3f& aabb) const;

private:
    std::array<Eigen::Hyperplane<float, 3>, 6> m_planes;
};

constexpr const Eigen::Hyperplane<float, 3>& Frustum::plane(unsigned index) const
{
    assert(index < m_planes.size());

    return m_planes[index];
}

#endif /* CCA9F659_CACE_4327_BC1D_1E658B29B0AB */
