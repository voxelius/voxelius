#ifndef D5554859_D83A_42F2_81FB_1BA81634255B
#define D5554859_D83A_42F2_81FB_1BA81634255B

#include "core/cvar/number.hh"

#include "shared/coord.hh"
#include "shared/net/protocol.hh"

class World;

namespace interest
{
extern cvar::Number<unsigned> view_distance;
} // namespace interest

namespace interest
{
void init(void);
void fixed_update_late(void);
} // namespace interest

namespace interest
{
template<typename T>
void broadcast(const World* world, const T& packet, const ChunkPos& origin, ENetPeer* except = nullptr);
template<typename T>
void broadcast(const World* world, const T& packet, ENetPeer* except = nullptr);
} // namespace interest

namespace interest
{
void mark_known(const World* world, entt::entity entity, const ChunkPos& chunk);
void mark_known(const World* world, entt::entity entity);
void forget(const World* world, entt::entity entity);
} // namespace interest

#endif /* D5554859_D83A_42F2_81FB_1BA81634255B */
