#ifndef CA457813_A93D_4AE7_8644_4B9FECFCD782
#define CA457813_A93D_4AE7_8644_4B9FECFCD782

#include "core/constant.hh"

namespace constant
{
constexpr static std::string_view BUILTIN_NAME_SPACE = "builtin";
} // namespace constant

namespace constant
{
constexpr static std::size_t CHUNK_SIZE = 16;
constexpr static std::size_t CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr static std::size_t CHUNK_VOLUME = CHUNK_AREA * CHUNK_SIZE;
constexpr static std::size_t CHUNK_SIZE_LOG2 = 4;
} // namespace constant

namespace constant
{
constexpr static std::int64_t SURFACE_MIN_Y = -64;
constexpr static std::int64_t SURFACE_MAX_Y = 255;
constexpr static std::int64_t SKY_MIN_Y = 256;
constexpr static std::int64_t SKY_MAX_Y = 767;
constexpr static std::int64_t UNDERGROUND_MIN_Y = -256;
constexpr static std::int64_t UNDERGROUND_MAX_Y = -65;
constexpr static std::int64_t THE_DEPTHS_MIN_Y = -512;
constexpr static std::int64_t THE_DEPTHS_MAX_Y = -257;

constexpr static std::size_t USERNAME_MAX_LENGTH = 64;
} // namespace constant

namespace constant
{
constexpr static std::int32_t SURFACE_MIN_CHUNK_Y = SURFACE_MIN_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t SURFACE_MAX_CHUNK_Y = SURFACE_MAX_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t SKY_MIN_CHUNK_Y = SKY_MIN_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t SKY_MAX_CHUNK_Y = SKY_MAX_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t UNDERGROUND_MIN_CHUNK_Y = UNDERGROUND_MIN_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t UNDERGROUND_MAX_CHUNK_Y = UNDERGROUND_MAX_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t THE_DEPTHS_MIN_CHUNK_Y = THE_DEPTHS_MIN_Y >> CHUNK_SIZE_LOG2;
constexpr static std::int32_t THE_DEPTHS_MAX_CHUNK_Y = THE_DEPTHS_MAX_Y >> CHUNK_SIZE_LOG2;
} // namespace constant

namespace constant
{
constexpr static std::uint64_t FIXED_TICKRATE = 20;
constexpr static std::uint64_t FIXED_FRAMETIME_US = 1000000 / FIXED_TICKRATE;
} // namespace constant

namespace constant
{
constexpr static std::uint64_t DAY_LENGTH_TICKS = 20 * 60 * FIXED_TICKRATE;
} // namespace constant

namespace constant
{
constexpr static float VELOCITY_EPS = 0.0001f;
constexpr static float VELOCITY_EPS_SQ = VELOCITY_EPS * VELOCITY_EPS;
} // namespace constant

#endif /* CA457813_A93D_4AE7_8644_4B9FECFCD782 */
