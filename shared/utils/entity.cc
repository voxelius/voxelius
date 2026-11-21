#include "shared/pch.hh"

#include "shared/utils/entity.hh"

#include "core/buffer.hh"
#include "core/exception.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/entity/required_class.hh"
#include "shared/utils/coord.hh"

static entt::entity create_entity(entt::registry& registry, entt::entity hint)
{
    if(hint == entt::null) {
        return registry.create();
    }
    else {
        auto result = registry.create(hint);

        if(result == hint) {
            return result;
        }

        registry.destroy(result);

        return entt::null;
    }
}

static void attach_class(entt::registry& registry, entt::entity entity, class_id_type id, Identifier name)
{
    EntityClass class_component {};
    class_component.id = id;
    class_component.name = std::move(name);
    registry.emplace<EntityClass>(entity, std::move(class_component));
}

static bool attach_components(entt::registry& registry, entt::entity entity, const ClassDefinition* def)
{
    for(auto& it : def->entries) {
        if(!component_map::attach(registry, it.id, entity, it.config)) {
            return false;
        }
    }

    return true;
}

static bool patch_components(entt::registry& registry, entt::entity entity, const ClassDefinition* def, lua_State* L, int kv_idx)
{
    for(auto& it : def->entries) {
        lua_getfield(L, kv_idx, it.name.c_str());

        if(!lua_isnil(L, -1)) {
            if(!component_map::patch(registry, it.id, entity, L, -1)) {
                lua_remove(L, -2);
                return false;
            }
        }
    }

    return true;
}

entt::entity utils::spawn(entt::registry& registry, class_id_type class_id, entt::entity hint)
{
    auto def = class_registry::find_definition(class_id);

    if(def == nullptr) {
        return entt::null;
    }

    auto entity = create_entity(registry, hint);

    if(!registry.valid(entity)) {
        msg::log_critical("if you're reading this, entt has fucked up");
        return entt::null;
    }

    auto name = class_registry::name_of(class_id);
    vx::throw_if_not(name.has_value(), "class_registry got corrupted");

    if(!attach_components(registry, entity, def)) {
        registry.destroy(entity);
        return entt::null;
    }

    attach_class(registry, entity, class_id, std::move(name.value()));

    return entity;
}

entt::entity utils::spawn(entt::registry& registry, class_id_type class_id, lua_State* L, int kv_idx, entt::entity hint)
{
    auto def = class_registry::find_definition(class_id);

    if(def == nullptr) {
        lua_pushstring(L, "unknown class id");
        return entt::null;
    }

    auto entity = create_entity(registry, hint);

    if(!registry.valid(entity)) {
        lua_pushstring(L, "if you're reading this, entt has fucked up");
        return entt::null;
    }

    auto name = class_registry::name_of(class_id);
    vx::throw_if_not(name.has_value(), "class_registry got corrupted");

    if(!attach_components(registry, entity, def)) {
        lua_pushstring(L, "failed to attach components");
        registry.destroy(entity);
        return entt::null;
    }

    attach_class(registry, entity, class_id, std::move(name.value()));

    if(!patch_components(registry, entity, def, L, kv_idx)) {
        lua_pushstring(L, "failed to patch components");
        registry.destroy(entity);
        return entt::null;
    }

    return entity;
}

entt::entity utils::spawn(entt::registry& registry, const Identifier& name, entt::entity hint)
{
    auto id = class_registry::find(name);

    return utils::spawn(registry, id, hint);
}

entt::entity utils::spawn(entt::registry& registry, const Identifier& name, lua_State* L, int kv_idx, entt::entity hint)
{
    auto id = class_registry::find(name);

    return utils::spawn(registry, id, L, kv_idx, hint);
}

entt::entity utils::spawn_player(entt::registry& registry, const BlockPos& pos, entt::entity hint)
{
    auto def = class_registry::find_definition(required_class::player);

    if(def == nullptr) {
        return entt::null;
    }

    auto entity = create_entity(registry, hint);

    if(!registry.valid(entity)) {
        msg::log_critical("if you're reading this, entt has fucked up");
        return entt::null;
    }

    auto player_class = class_registry::name_of(required_class::player);
    vx::throw_if_not(player_class.has_value(), "class_registry got corrupted");

    if(!attach_components(registry, entity, def)) {
        registry.destroy(entity);
        return entt::null;
    }

    attach_class(registry, entity, required_class::player, std::move(player_class.value()));

    if(registry.all_of<Transform>(entity)) {
        registry.patch<Transform>(entity, [&](Transform& transform) {
            auto local_pos = utils::to_local(pos);
            transform.chunk = utils::to_chunk(pos);
            transform.local = local_pos.cast<float>();
        });
    }

    return entity;
}

entt::entity utils::transfer(entt::registry& from, entt::entity entity, entt::registry& to)
{
    auto class_component = from.try_get<EntityClass>(entity);

    if(class_component == nullptr) {
        return entt::null;
    }

    struct SnapshotEntry final {
        component_id_type id;
        std::vector<std::byte> data;
    };

    std::vector<SnapshotEntry> snapshot;

    class_registry::for_each(class_component->id, [&](component_id_type id) {
        WriteBuffer buffer;
        component_map::encode_net(from, id, entity, buffer);

        SnapshotEntry entry {};
        entry.id = id;
        entry.data = buffer.take();

        snapshot.emplace_back(std::move(entry));
    });

    auto result = utils::spawn(to, class_component->id);

    if(!to.valid(result)) {
        return entt::null;
    }

    for(auto& entry : snapshot) {
        ReadBuffer buffer(std::span<const std::byte>(entry.data));
        component_map::decode_net(to, entry.id, result, buffer);
    }

    from.destroy(entity);

    return result;
}
