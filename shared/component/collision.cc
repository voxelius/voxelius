#include "shared/pch.hh"

#include "shared/component/collision.hh"

#include "core/buffer.hh"
#include "core/utils/crc64.hh"

#include "shared/entity/component_map.hh"
#include "shared/utils/lua.hh"

static bool has_field(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto present = !lua_isnil(L, -1);

    lua_pop(L, 1);

    return present;
}

static std::optional<Eigen::AlignedBox3f> parse_box(lua_State* L, int config_idx)
{
    auto min_value = utils::require_fvec<3>(L, config_idx, "min");
    auto max_value = utils::require_fvec<3>(L, config_idx, "max");

    if(!min_value.has_value() || !max_value.has_value()) {
        return std::nullopt;
    }

    Eigen::AlignedBox3f box;
    box.min() = min_value->cast<float>();
    box.max() = max_value->cast<float>();

    return box;
}

static bool valid_box(const Eigen::AlignedBox3f& box)
{
    return box.min().x() < box.max().x() && box.min().y() < box.max().y() && box.min().z() < box.max().z();
}

static Eigen::AlignedBox3f sized_box(float width, float height)
{
    Eigen::AlignedBox3f box;
    box.min() = Eigen::Vector3f(-0.5f * width, 0.0f, -0.5f * width);
    box.max() = Eigen::Vector3f(0.5f * width, height, 0.5f * width);

    return box;
}

template<>
std::any Component<Collision>::prepare(lua_State* L, int config_idx)
{
    auto step_height = utils::require_number(L, config_idx, "step_height");

    if(!step_height.has_value()) {
        return std::any {};
    }

    auto step_height_value = static_cast<float>(step_height.value());

    if(step_height_value < 0.0f) {
        return std::any {};
    }

    auto has_min = has_field(L, config_idx, "min");
    auto has_max = has_field(L, config_idx, "max");

    if(has_min != has_max) {
        return std::any {};
    }

    Eigen::AlignedBox3f result;

    if(has_min) {
        auto box = parse_box(L, config_idx);

        if(!box.has_value()) {
            return std::any {};
        }

        result = box.value();
    }
    else {
        auto width = utils::require_number(L, config_idx, "width");
        auto height = utils::require_number(L, config_idx, "height");

        if(!width.has_value() || !height.has_value()) {
            return std::any {};
        }

        auto width_value = static_cast<float>(width.value());
        auto height_value = static_cast<float>(height.value());

        if(width_value <= 0.0f || height_value <= 0.0f) {
            return std::any {};
        }

        result = sized_box(width_value, height_value);
    }

    if(!valid_box(result)) {
        return std::any {};
    }

    Collision collision;
    collision.local_box = result;
    collision.step_height = step_height_value;

    return collision;
}

template<>
void Component<Collision>::hash_config(std::uint64_t& hash, const std::any& config)
{
    auto value = std::any_cast<Collision>(&config);

    if(value == nullptr) {
        return;
    }

    hash = utils::crc64(&value->local_box.min(), sizeof(value->local_box.min()), hash);
    hash = utils::crc64(&value->local_box.max(), sizeof(value->local_box.max()), hash);
    hash = utils::crc64(&value->step_height, sizeof(value->step_height), hash);
}

template<>
void Component<Collision>::connect(entt::registry& registry)
{
    registry.on_update<Collision>().connect<&component_map::on_update<Collision>>();
}

template<>
void Component<Collision>::attach(entt::registry& registry, entt::entity entity, const std::any& config)
{
    auto value = std::any_cast<Collision>(&config);

    assert(value);

    registry.emplace_or_replace<Collision>(entity, *value);
}

template<>
bool Component<Collision>::patch(entt::registry& registry, entt::entity entity, lua_State* L, int kv_idx)
{
    auto& current = registry.get<Collision>(entity);

    auto step_height = utils::opt_number(L, kv_idx, "step_height", current.step_height);

    if(!step_height.has_value()) {
        return false;
    }

    auto step_height_value = static_cast<float>(step_height.value());

    if(step_height_value < 0.0f) {
        return false;
    }

    auto has_min = has_field(L, kv_idx, "min");
    auto has_max = has_field(L, kv_idx, "max");

    if(has_min != has_max) {
        return false;
    }

    Eigen::AlignedBox3f next = current.local_box;

    if(has_min) {
        auto box = parse_box(L, kv_idx);

        if(!box.has_value()) {
            return false;
        }

        next = box.value();
    }
    else if(has_field(L, kv_idx, "width") || has_field(L, kv_idx, "height")) {
        auto width = utils::opt_number(L, kv_idx, "width", current.local_box.sizes().x());
        auto height = utils::opt_number(L, kv_idx, "height", current.local_box.sizes().y());

        if(!width.has_value() || !height.has_value()) {
            return false;
        }

        auto width_value = static_cast<float>(width.value());
        auto height_value = static_cast<float>(height.value());

        if(width_value <= 0.0f || height_value <= 0.0f) {
            return false;
        }

        next = sized_box(width_value, height_value);
    }

    if(!valid_box(next)) {
        return false;
    }

    registry.patch<Collision>(entity, [&](Collision& collision) {
        collision.local_box = next;
        collision.step_height = step_height_value;
    });

    return true;
}

template<>
void Component<Collision>::encode_net(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    const auto& collision = registry.get<Collision>(entity);

    buffer.write_vector<float, 3>(collision.local_box.min());
    buffer.write_vector<float, 3>(collision.local_box.max());
    buffer.write<float>(collision.step_height);
}

template<>
void Component<Collision>::decode_net(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    registry.patch<Collision>(entity, [&](Collision& collision) {
        auto min_value = buffer.read_vector<float, 3>();
        auto max_value = buffer.read_vector<float, 3>();

        collision.local_box.min() = min_value;
        collision.local_box.max() = max_value;
        collision.step_height = buffer.read<float>();
    });
}

template<>
void Component<Collision>::encode_dat(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    encode_net(registry, entity, buffer);
}

template<>
void Component<Collision>::decode_dat(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    decode_net(registry, entity, buffer);
}

void Collision::register_component(void)
{
    component_map::add<Collision>("collision");
}
