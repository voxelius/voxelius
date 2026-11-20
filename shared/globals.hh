#ifndef E3CA7055_91BF_4C7D_ACF9_C112C07BA574
#define E3CA7055_91BF_4C7D_ACF9_C112C07BA574

class World;

namespace globals
{
extern entt::dispatcher dispatcher;
} // namespace globals

namespace globals
{
extern World* world;
} // namespace globals

namespace globals
{
extern std::uint64_t curtime_us;
extern std::uint64_t current_tick;
extern std::uint64_t daylight_tick;
extern std::uint64_t day;
} // namespace globals

namespace globals
{
extern std::size_t fixed_framecount;
extern std::uint64_t fixed_frametime_us;
extern float fixed_frametime;
extern float fixed_frametime_avg;
} // namespace globals

#endif /* E3CA7055_91BF_4C7D_ACF9_C112C07BA574 */
