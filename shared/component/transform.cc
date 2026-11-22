#include "shared/pch.hh"

#include "shared/component/transform.hh"

#include "core/buffer.hh"

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/entity/class.hh"
#include "shared/entity/component_map.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/lua.hh"

constexpr inline static bool update_component(unsigned dim, Transform& component)
{
    if(component.local[dim] >= constant::CHUNK_SIZE) {
        component.local[dim] -= constant::CHUNK_SIZE;
        component.chunk[dim] += 1;
        return true;
    }

    if(component.local[dim] < 0.0f) {
        component.local[dim] += constant::CHUNK_SIZE;
        component.chunk[dim] -= 1;
        return true;
    }

    return false;
}

template<>
std::any Component<Transform>::prepare(lua_State*, int)
{
    return std::monostate {};
}

template<>
void Component<Transform>::connect(entt::registry& registry)
{
    registry.on_update<Transform>().connect<&component_map::on_update<Transform>>();
}

template<>
void Component<Transform>::attach(entt::registry& registry, entt::entity entity, const std::any&)
{
    Transform transform {};
    transform.chunk = ChunkPos::Zero();
    transform.local = Eigen::Vector3f::Zero();
    transform.angles = Eigen::Vector3f::Zero();

    registry.emplace_or_replace<Transform>(entity, std::move(transform));
}

template<>
bool Component<Transform>::patch(entt::registry& registry, entt::entity entity, lua_State* L, int kv_idx)
{
    auto& current = registry.get<Transform>(entity);
    auto current_bpos = utils::to_block(current.chunk, current.local.cast<ChunkPos::value_type>());

    auto bpos = utils::opt_ivec<3>(L, kv_idx, "bpos", current_bpos.cast<lua_Integer>());

    if(!bpos.has_value()) {
        return false;
    }

    auto angs = utils::opt_fvec<3>(L, kv_idx, "angs", current.angles.cast<lua_Number>());

    if(!angs.has_value()) {
        return false;
    }

    auto bpos_value = bpos.value();
    auto bpos_block = bpos_value.cast<BlockPos::value_type>();
    auto bpos_chunk = utils::to_chunk(bpos_block);
    auto bpos_local = utils::to_local(bpos_block);
    auto angs_value = angs.value();

    registry.patch<Transform>(entity, [&](Transform& transform) {
        transform.chunk = bpos_chunk;
        transform.local = bpos_local.cast<float>();
        transform.angles = angs_value.cast<float>();
    });

    return true;
}

template<>
void Component<Transform>::encode_net(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    const auto& transform = registry.get<Transform>(entity);
    buffer.write_vector<std::int64_t, 3>(transform.chunk.cast<std::int64_t>());
    buffer.write_vector<float, 3>(transform.local);
    buffer.write_vector<float, 3>(transform.angles);
}

template<>
void Component<Transform>::decode_net(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    registry.patch<Transform>(entity, [&](Transform& transform) {
        auto chunk_value = buffer.read_vector<std::int64_t, 3>();
        transform.chunk = chunk_value.cast<ChunkPos::value_type>();
        transform.local = buffer.read_vector<float, 3>();
        transform.angles = buffer.read_vector<float, 3>();
    });
}

template<>
void Component<Transform>::encode_dat(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    encode_net(registry, entity, buffer);
}

template<>
void Component<Transform>::decode_dat(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    decode_net(registry, entity, buffer);
}

template<>
void Component<Transform>::interpolate(entt::registry& registry, entt::entity entity, float alpha)
{
    auto current = registry.try_get<Transform>(entity);
    auto previous = registry.try_get<Transform_Prev>(entity);
    auto interp = registry.try_get<Transform_Intr>(entity);

    if(current == nullptr || previous == nullptr || interp == nullptr) {
        return;
    }

    interp->angles[0] = std::lerp(previous->angles[0], current->angles[0], alpha);
    interp->angles[1] = std::lerp(previous->angles[1], current->angles[1], alpha);
    interp->angles[2] = std::lerp(previous->angles[2], current->angles[2], alpha);

    // Figure out previous chunk-local floating-point coordinates transformed
    // to the current WorldCoord's chunk domain coordinates; we're interpolating
    // against these instead of using previous.position.local to prevent jittering
    auto previous_shift = utils::to_relative(current->chunk, current->local, previous->chunk, previous->local);
    auto previous_local = current->local + previous_shift;

    interp->chunk = current->chunk;

    interp->local.x() = std::lerp(previous_local.x(), current->local.x(), alpha);
    interp->local.y() = std::lerp(previous_local.y(), current->local.y(), alpha);
    interp->local.z() = std::lerp(previous_local.z(), current->local.z(), alpha);
}

void Transform::register_component(void)
{
    component_map::add<Transform>("transform");
}

void Transform::fixed_update(entt::registry& registry)
{
    auto view = registry.view<Transform>();

    for(auto [entity, transform] : view.each()) {
        auto changed = false;
        changed = update_component(0, transform) || changed;
        changed = update_component(1, transform) || changed;
        changed = update_component(2, transform) || changed;

        if(changed) {
            registry.patch<Transform>(entity);
        }
    }
}
