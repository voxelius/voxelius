#ifndef F8834F01_219B_4D3A_86E7_91FA52B4F625
#define F8834F01_219B_4D3A_86E7_91FA52B4F625

#include "core/cvar/number.hh"

#include "shared/coord.hh"
#include "shared/world/block.hh"
#include "shared/world/fluid.hh"

class Camera;

namespace client_camera
{
extern cvar::Number<float> vertical_fov;
extern cvar::Number<unsigned> view_distance;
extern cvar::Number<unsigned> cloud_range;
} // namespace client_camera

namespace client_camera
{
extern Camera instance;
} // namespace client_camera

namespace client_camera
{
extern Eigen::Vector3f angles;
extern Eigen::Vector3f forward;
extern Eigen::Vector3f right;
extern Eigen::Vector3f up;
extern Eigen::Vector3f local;
extern ChunkPos chunk;
} // namespace client_camera

namespace client_camera
{
extern float z_near;
extern float z_far;
} // namespace client_camera

namespace client_camera
{
extern block_id_type inside_block;
extern fluid_id_type inside_fluid;
} // namespace client_camera

namespace client_camera
{
// Horizontal direction of travel, scaled to the given number of chunks.
// Chunk work is ordered towards it, so while flying the leading edge is
// fetched and meshed before the ground that is already behind. A player
// below the moving-speed threshold yields zero
Eigen::Vector3f lookahead(float chunks);
} // namespace client_camera

namespace client_camera
{
void init(void);
void update(void);
} // namespace client_camera

#endif /* F8834F01_219B_4D3A_86E7_91FA52B4F625 */
