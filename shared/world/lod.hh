#ifndef B7F3A1C4_5D82_4E69_9B0A_C1D2E3F40516
#define B7F3A1C4_5D82_4E69_9B0A_C1D2E3F40516

namespace lod
{
constexpr static std::uint8_t COLOR_PALETTE8 = 0;
constexpr static std::uint8_t COLOR_RGB332 = 1;
constexpr static std::size_t PALETTE_MAX = 255;
} // namespace lod

using LodPalette = std::vector<std::array<std::uint8_t, 3>>;

namespace lod
{
std::array<std::uint8_t, 3> to_rgb8(const Eigen::Vector3f& color);
std::uint8_t to_rgb332(const Eigen::Vector3f& color);
Eigen::Vector3f from_rgb332(std::uint8_t color);
Eigen::Vector3f unpack(std::uint8_t mode, const LodPalette& palette, std::uint8_t index);
} // namespace lod

#endif /* B7F3A1C4_5D82_4E69_9B0A_C1D2E3F40516 */
