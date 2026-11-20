#ifndef AE386242_54A6_40A1_A284_58C367BBC53C
#define AE386242_54A6_40A1_A284_58C367BBC53C

namespace world_clock
{
// Where the day is, from 0 at sunrise to 0.5 at sunset.
// The phase wraps, so a tick past a whole day starts over
float phase(std::uint64_t daylight_tick, std::uint32_t day_length);
} // namespace world_clock

namespace world_clock
{
// Whole days since the world clock started
std::uint64_t day(std::uint64_t daylight_tick, std::uint32_t day_length);
} // namespace world_clock

namespace world_clock
{
// How bright the world is, from 0.12 at midnight to 1 at noon
float daylight(float phase);
} // namespace world_clock

namespace world_clock
{
// Unit vector towards the sun for a phase;
// 0 is due east, 0.25 the zenith and 0.5 due west
Eigen::Vector3f sun_direction(float phase);
} // namespace world_clock

#endif /* AE386242_54A6_40A1_A284_58C367BBC53C */
