#ifndef AD54D621_66CB_4B08_A298_C58D9FD8CE86
#define AD54D621_66CB_4B08_A298_C58D9FD8CE86

namespace utils
{
float wrap_angle_360(float angle_rad);
float wrap_angle_180(float angle_rad);
} // namespace utils

namespace utils
{
Eigen::Vector3f wrap_angle_360(Eigen::Vector3f angle_rad);
Eigen::Vector3f wrap_angle_180(Eigen::Vector3f angle_rad);
} // namespace utils

namespace utils
{
constexpr float radians(float degrees);
constexpr float degrees(float radians);
} // namespace utils

namespace utils
{
template<typename T, int N>
constexpr Eigen::Vector<T, N> radians(const Eigen::Vector<T, N>& degrees);
template<typename T, int N>
constexpr Eigen::Vector<T, N> degrees(const Eigen::Vector<T, N>& radians);
} // namespace utils

constexpr float utils::radians(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

constexpr float utils::degrees(float radians)
{
    return radians * 180.0f / std::numbers::pi_v<float>;
}

template<typename T, int N>
constexpr Eigen::Vector<T, N> utils::radians(const Eigen::Vector<T, N>& degrees)
{
    return degrees * static_cast<T>(std::numbers::pi_v<float> / 180.0f);
}

template<typename T, int N>
constexpr Eigen::Vector<T, N> utils::degrees(const Eigen::Vector<T, N>& radians)
{
    return radians * static_cast<T>(180.0f / std::numbers::pi_v<float>);
}

#endif /* AD54D621_66CB_4B08_A298_C58D9FD8CE86 */
