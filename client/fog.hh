#ifndef CD23284B_9DDA_4DC2_A40E_8188E1120367
#define CD23284B_9DDA_4DC2_A40E_8188E1120367

#include "core/cvar/number.hh"

namespace fog
{
extern float distance;
extern Eigen::Vector3f color;

// Scales the sky, the fog and the light the sky casts at once. One knob for
// how bright the day is: 1 is noon, well below that is dusk.
extern cvar::Number<float> daylight;
} // namespace fog

namespace fog
{
void init(void);
} // namespace fog

namespace fog
{
void update(void);
} // namespace fog

#endif /* CD23284B_9DDA_4DC2_A40E_8188E1120367 */
