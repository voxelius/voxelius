#include "shared/pch.hh"

#include "shared/component/player.hh"

#include "shared/entity/component_map.hh"

template<>
std::any Component<Player>::prepare(lua_State* L, int config_idx)
{
    return std::monostate {};
}

template<>
void Component<Player>::attach(entt::registry& registry, entt::entity entity, const std::any& config)
{
    registry.emplace<Player>(entity);
}

template<>
bool Component<Player>::patch(entt::registry& registry, entt::entity entity, lua_State* L, int kv_idx)
{
    return true;
}

template<>
void Component<Player>::encode_net(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    // empty
}

template<>
void Component<Player>::decode_net(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    // empty
}

template<>
void Component<Player>::encode_dat(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    // empty
}

template<>
void Component<Player>::decode_dat(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    // empty
}

template<>
void Component<Player>::connect(entt::registry& registry)
{
    registry.on_update<Player>().connect<&component_map::on_update<Player>>();
}

void Player::register_component(void)
{
    component_map::add<Player>("player");
}
