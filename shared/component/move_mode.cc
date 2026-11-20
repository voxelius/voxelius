#include "shared/pch.hh"

#include "shared/component/move_mode.hh"

#include "core/buffer.hh"

#include "shared/entity/class.hh"
#include "shared/entity/component_map.hh"
#include "shared/utils/lua.hh"

static std::string_view value_to_string(unsigned value)
{
    switch(value) {
        case MoveMode::FLY:
            return std::string_view("FLY");

        case MoveMode::WALK:
            return std::string_view("WALK");
    }

    return {};
}

static std::optional<unsigned> value_from_string(std::string_view str)
{
    if(str == "FLY") {
        return MoveMode::FLY;
    }

    if(str == "WALK") {
        return MoveMode::WALK;
    }

    return std::nullopt;
}

template<>
std::any Component<MoveMode>::prepare(lua_State* L, int config_idx)
{
    auto value_str = utils::opt_string(L, config_idx, "value", value_to_string(MoveMode::WALK));

    if(!value_str.has_value()) {
        return std::any {};
    }

    auto value = value_from_string(value_str.value());

    if(!value.has_value()) {
        return std::any {};
    }

    return value.value();
}

template<>
void Component<MoveMode>::attach(entt::registry& registry, entt::entity entity, const std::any& config)
{
    assert(std::any_cast<const unsigned>(&config));

    MoveMode move_mode {};
    move_mode.value = std::any_cast<const unsigned>(config);

    registry.emplace_or_replace<MoveMode>(entity, std::move(move_mode));
}

template<>
bool Component<MoveMode>::patch(entt::registry& registry, entt::entity entity, lua_State* L, int kv_idx)
{
    auto& current = registry.get<MoveMode>(entity);

    auto value_str = utils::opt_string(L, kv_idx, "value", value_to_string(current.value));

    if(!value_str.has_value()) {
        return false;
    }

    auto value = value_from_string(value_str.value());

    if(!value.has_value()) {
        lua_pushstring(L, "invalid MoveMode: ");
        lua_pushlstring(L, value_str->data(), value_str->size());
        lua_concat(L, 2);
        return false;
    }

    registry.patch<MoveMode>(entity, [&](MoveMode& move_mode) {
        move_mode = MoveMode(value.value());
    });

    return true;
}

template<>
void Component<MoveMode>::encode_net(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    const auto& move_mode = registry.get<MoveMode>(entity);
    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(move_mode.value));
}

template<>
void Component<MoveMode>::decode_net(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    registry.patch<MoveMode>(entity, [&](MoveMode& move_mode) {
        move_mode.value = static_cast<unsigned>(buffer.read<std::uint8_t>());
    });
}

template<>
void Component<MoveMode>::encode_dat(entt::registry& registry, entt::entity entity, WriteBuffer& buffer)
{
    encode_net(registry, entity, buffer);
}

template<>
void Component<MoveMode>::decode_dat(entt::registry& registry, entt::entity entity, ReadBuffer& buffer)
{
    decode_net(registry, entity, buffer);
}

template<>
void Component<MoveMode>::connect(entt::registry& registry)
{
    registry.on_update<MoveMode>().connect<&component_map::on_update<MoveMode>>();
}

void MoveMode::register_component(void)
{
    component_map::add<MoveMode>("move_mode");
}
