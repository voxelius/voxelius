#include "shared/pch.hh"

#include "shared/component/velocity.hh"

#include "core/buffer.hh"

#include "shared/component/stasis.hh"
#include "shared/component/transform.hh"
#include "shared/constant.hh"
#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/utils/lua.hh"

template<>
std::any Component<Velocity>::prepare(lua_State* L, int config_idx)
{
    return std::monostate {};
}

template<>
void Component<Velocity>::attach(entt::entity entity, const std::any& config)
{
    Velocity velocity {};
    velocity.value = Eigen::Vector3f::Zero();

    globals::registry.emplace_or_replace<Velocity>(entity, std::move(velocity));
}

template<>
bool Component<Velocity>::patch(entt::entity entity, lua_State* L, int kv_idx)
{
    auto& current = globals::registry.get<Velocity>(entity);
    auto value = utils::opt_fvec<3>(L, kv_idx, current.value.cast<lua_Number>());

    if(!value.has_value()) {
        return false;
    }

    globals::registry.patch<Velocity>(entity, [&](Velocity& velocity) {
        velocity.value = value.value().cast<float>();
    });

    return true;
}

template<>
void Component<Velocity>::encode_net(entt::entity entity, WriteBuffer& buffer)
{
    const auto& velocity = globals::registry.get<Velocity>(entity);
    buffer.write_vector<float, 3>(velocity.value);
}

template<>
void Component<Velocity>::decode_net(entt::entity entity, ReadBuffer& buffer)
{
    globals::registry.patch<Velocity>(entity, [&](Velocity& velocity) {
        velocity.value = buffer.read_vector<float, 3>();
    });
}

template<>
void Component<Velocity>::encode_dat(entt::entity entity, WriteBuffer& buffer)
{
    encode_net(entity, buffer);
}

template<>
void Component<Velocity>::decode_dat(entt::entity entity, ReadBuffer& buffer)
{
    decode_net(entity, buffer);
}

template<>
void Component<Velocity>::interpolate(entt::entity entity, float alpha)
{
    auto* current = globals::registry.try_get<Velocity>(entity);
    auto* previous = globals::registry.try_get<Velocity_Prev>(entity);
    auto* interp = globals::registry.try_get<Velocity_Intr>(entity);

    if(current == nullptr || previous == nullptr || interp == nullptr) {
        return;
    }

    interp->value.x() = std::lerp(previous->value.x(), current->value.x(), alpha);
    interp->value.y() = std::lerp(previous->value.y(), current->value.y(), alpha);
    interp->value.z() = std::lerp(previous->value.z(), current->value.z(), alpha);
}

void Velocity::register_component(void)
{
    component_map::add<Velocity>("velocity");

    globals::registry.on_update<Velocity>().connect<&component_map::on_update<Velocity>>();
}

void Velocity::fixed_update(void)
{
    auto view = globals::registry.view<Velocity, Transform>(entt::exclude<Stasis>);

    for(auto [entity, velocity, transform] : view.each()) {
        auto speed_sq = velocity.value.squaredNorm();

        if(speed_sq >= constant::VELOCITY_EPS_SQ) {
            transform.local += velocity.value * globals::fixed_frametime;

            globals::registry.patch<Transform>(entity);
        }
        else if(speed_sq) {
            velocity.value.setZero();

            globals::registry.patch<Velocity>(entity);
        }
    }
}
