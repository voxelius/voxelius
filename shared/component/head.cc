#include "shared/pch.hh"

#include "shared/component/head.hh"

#include "core/buffer.hh"

#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/utils/lua.hh"

template<>
std::any Component<Head>::prepare(lua_State* L, int config_idx)
{
    auto offset = utils::opt_fvec<3>(L, config_idx, { 0.0, 0.0, 0.0 });

    if(!offset.has_value()) {
        return std::any {};
    }

    return Eigen::Vector3f(offset.value().cast<float>());
}

template<>
void Component<Head>::attach(entt::entity entity, const std::any& config)
{
    assert(std::any_cast<const Eigen::Vector3f>(&config));

    Head head {};
    head.offset = std::any_cast<const Eigen::Vector3f>(config);
    head.angles = Eigen::Vector3f::Zero();

    globals::registry.emplace_or_replace<Head>(entity, std::move(head));
}

template<>
bool Component<Head>::patch(entt::entity entity, lua_State* L, int kv_idx)
{
    auto& current = globals::registry.get<Head>(entity);
    auto angles = utils::opt_fvec<3>(L, kv_idx, current.angles.cast<lua_Number>());

    if(!angles.has_value()) {
        return false;
    }

    globals::registry.patch<Head>(entity, [&](Head& head) {
        head.angles = angles.value().cast<float>();
    });

    return true;
}

template<>
void Component<Head>::encode_net(entt::entity entity, WriteBuffer& buffer)
{
    const auto& head = globals::registry.get<Head>(entity);
    buffer.write_vector<float, 3>(head.angles);
}

template<>
void Component<Head>::decode_net(entt::entity entity, ReadBuffer& buffer)
{
    globals::registry.patch<Head>(entity, [&](Head& head) {
        head.angles = buffer.read_vector<float, 3>();
    });
}

template<>
void Component<Head>::encode_dat(entt::entity entity, WriteBuffer& buffer)
{
    const auto& head = globals::registry.get<Head>(entity);
    buffer.write_vector<float, 3>(head.angles);
    buffer.write_vector<float, 3>(head.offset);
}

template<>
void Component<Head>::decode_dat(entt::entity entity, ReadBuffer& buffer)
{
    globals::registry.patch<Head>(entity, [&](Head& head) {
        head.angles = buffer.read_vector<float, 3>();
        head.offset = buffer.read_vector<float, 3>();
    });
}

template<>
void Component<Head>::interpolate(entt::entity entity, float alpha)
{
    auto* current = globals::registry.try_get<Head>(entity);
    auto* previous = globals::registry.try_get<Head_Prev>(entity);
    auto* interp = globals::registry.try_get<Head_Intr>(entity);

    if(current == nullptr || previous == nullptr || interp == nullptr) {
        return;
    }

    interp->angles[0] = std::lerp(previous->angles[0], current->angles[0], alpha);
    interp->angles[1] = std::lerp(previous->angles[1], current->angles[1], alpha);
    interp->angles[2] = std::lerp(previous->angles[2], current->angles[2], alpha);

    interp->offset.x() = std::lerp(previous->offset.x(), current->offset.x(), alpha);
    interp->offset.y() = std::lerp(previous->offset.y(), current->offset.y(), alpha);
    interp->offset.z() = std::lerp(previous->offset.z(), current->offset.z(), alpha);
}

void Head::register_component(void)
{
    component_map::add<Head>("head");

    globals::registry.on_update<Head>().connect<&component_map::on_update<Head>>();
}
