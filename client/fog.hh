#ifndef CD23284B_9DDA_4DC2_A40E_8188E1120367
#define CD23284B_9DDA_4DC2_A40E_8188E1120367

#include "core/concepts.hh"

namespace cvar
{
template<vx::arithmetic T>
class Number;
} // namespace cvar

namespace fog
{
extern cvar::Number<unsigned> model;
extern float distance;
extern Eigen::Vector3f color;
extern float light_scale;
extern float daylight;
} // namespace fog

namespace fog
{
void init(void);
void update(void);
void reset(void);
} // namespace fog

#endif /* CD23284B_9DDA_4DC2_A40E_8188E1120367 */
