#ifndef CD23284B_9DDA_4DC2_A40E_8188E1120367
#define CD23284B_9DDA_4DC2_A40E_8188E1120367

#include "core/cvar/number.hh"

namespace fog
{
extern float distance;
extern Eigen::Vector3f color;
extern float light_scale;
} // namespace fog

namespace fog
{
extern cvar::Number<float> daylight; // 0 = night; 1 = noon
} // namespace fog

namespace fog
{
void init(void);
void update(void);
void reset(void);
} // namespace fog

#endif /* CD23284B_9DDA_4DC2_A40E_8188E1120367 */
