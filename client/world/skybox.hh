#ifndef C3190BCB_73A1_4C71_8EC2_3A63242769EB
#define C3190BCB_73A1_4C71_8EC2_3A63242769EB

namespace skybox
{
extern Eigen::Vector3f horizon_color;
extern Eigen::Vector3f sun_horizon_color;
extern Eigen::Vector3f fog_color;
extern Eigen::Vector3f zenith_color;

extern Eigen::Vector3f sun_direction;
extern Eigen::Vector2f sun_azimuth;
extern Eigen::Vector3f sun_tint;
extern float sun_strength;

extern Eigen::Vector3f moon_direction;
extern Eigen::Vector3f moon_tint;
extern float moon_strength;
extern std::uint32_t moon_phase;

extern float sun_tilt;
extern float scatter;

extern float phase;
extern float daylight;
extern float light_scale;
extern float star_visibility;
extern std::uint32_t star_seed;
} // namespace skybox

namespace skybox
{
void init(void);
void update(void);
} // namespace skybox

namespace skybox
{
void reset(void);
void snap(void);
} // namespace skybox

namespace skybox
{
void set_star_seed(std::uint32_t seed);
} // namespace skybox

#endif /* C3190BCB_73A1_4C71_8EC2_3A63242769EB */
