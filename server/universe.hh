#ifndef E5FBE30D_A589_4737_8B37_7DE782E75B94
#define E5FBE30D_A589_4737_8B37_7DE782E75B94

#include "core/identifier.hh"

class World;

namespace universe
{
std::span<const std::shared_ptr<World>> worlds(void);
std::shared_ptr<World> find(const Identifier& id);
const std::shared_ptr<World>& default_world(void);
} // namespace universe

namespace universe
{
std::uint64_t seed(void);
} // namespace universe

namespace universe
{
void init(void);
void init_late(void);
void shutdown(void);
} // namespace universe

#endif /* E5FBE30D_A589_4737_8B37_7DE782E75B94 */
