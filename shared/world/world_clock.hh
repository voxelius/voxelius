#ifndef AE386242_54A6_40A1_A284_58C367BBC53C
#define AE386242_54A6_40A1_A284_58C367BBC53C

namespace world_clock
{
float phase(std::uint64_t daylight_tick, std::uint32_t day_length);
} // namespace world_clock

namespace world_clock
{
std::uint64_t day(std::uint64_t daylight_tick, std::uint32_t day_length);
} // namespace world_clock

namespace world_clock
{
float daylight(float phase);
} // namespace world_clock

namespace world_clock
{
Eigen::Vector3f sun_direction(float phase);
} // namespace world_clock

#endif /* AE386242_54A6_40A1_A284_58C367BBC53C */
