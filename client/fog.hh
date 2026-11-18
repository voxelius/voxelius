#ifndef CD23284B_9DDA_4DC2_A40E_8188E1120367
#define CD23284B_9DDA_4DC2_A40E_8188E1120367

namespace fog
{
extern float distance;
extern Eigen::Vector3f color;
extern float light_scale;
extern float daylight;
} // namespace fog

namespace fog
{
void update(void);
void reset(void);
} // namespace fog

#endif /* CD23284B_9DDA_4DC2_A40E_8188E1120367 */
