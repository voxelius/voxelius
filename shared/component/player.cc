#include "shared/pch.hh"

#include "shared/component/player.hh"

#include "shared/entity/component_map.hh"

template<>
std::any Component<Player>::prepare(lua_State*, int)
{
    return std::monostate {};
}

template<>
void Component<Player>::connect(entt::registry& registry)
{
    auto on_update = registry.on_update<Player>();
    on_update.connect<&component_map::on_update<Player>>();
}

template<>
void Component<Player>::attach(entt::registry& registry, entt::entity entity, const std::any&)
{
    registry.emplace<Player>(entity);
}

template<>
bool Component<Player>::patch(entt::registry&, entt::entity, lua_State*, int)
{
    return true;
}

template<>
void Component<Player>::encode_net(entt::registry&, entt::entity, WriteBuffer&)
{
    // empty
}

template<>
void Component<Player>::decode_net(entt::registry&, entt::entity, ReadBuffer&)
{
    // empty
}

template<>
void Component<Player>::encode_dat(entt::registry&, entt::entity, WriteBuffer&)
{
    // empty
}

template<>
void Component<Player>::decode_dat(entt::registry&, entt::entity, ReadBuffer&)
{
    // empty
}

void Player::register_component(void)
{
    component_map::add<Player>("player");
}
