#ifndef FEDE636E_EA8B_465F_8397_6BDB599AA241
#define FEDE636E_EA8B_465F_8397_6BDB599AA241

#include "shared/coord.hh"
#include "shared/entity/class.hh"

class Identifier;

namespace utils
{
entt::entity spawn(entt::registry& registry, class_id_type class_id, entt::entity hint = entt::null);
entt::entity spawn(entt::registry& registry, class_id_type class_id, lua_State* L, int kv_idx, entt::entity hint = entt::null);
} // namespace utils

namespace utils
{
entt::entity spawn(entt::registry& registry, const Identifier& name, entt::entity hint = entt::null);
entt::entity spawn(entt::registry& registry, const Identifier& name, lua_State* L, int kv_idx, entt::entity hint = entt::null);
} // namespace utils

namespace utils
{
entt::entity spawn_player(entt::registry& registry, const BlockPos& pos, entt::entity hint = entt::null);
} // namespace utils

namespace utils
{
// Moves an entity to another registry and returns the new entity. The source
// entity is destroyed. The caller places the new entity.
entt::entity transfer(entt::registry& from, entt::entity entity, entt::registry& to);
} // namespace utils

#endif /* FEDE636E_EA8B_465F_8397_6BDB599AA241 */
