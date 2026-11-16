#include "server/pch.hh"

#include "server/system/collector.hh"

#include "shared/component/transform.hh"
#include "shared/entity/class.hh"
#include "shared/entity/component_map.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/protocol.hh"

#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/sessions.hh"

static void on_create_entity(entt::registry& registry, entt::entity entity)
{
    assert(&registry == &globals::registry); // sanyaty check

    auto& component = registry.get<EntityClass>(entity);

    packet::Entity_Spawn packet {};
    packet.entity = entity;
    packet.class_id = component.id;

    if(auto transform = registry.try_get<Transform>(entity)) {
        interest::broadcast(packet, transform->chunk);
        interest::mark_known(entity, transform->chunk);
    }
    else {
        protocol::broadcast(packet, globals::host);
        interest::mark_known(entity);
    }
}

static void on_destroy_entity(entt::registry& registry, entt::entity entity)
{
    assert(&registry == &globals::registry); // sanyaty check

    packet::Entity_Remove packet {};
    packet.entity = entity;

    if(auto transform = registry.try_get<Transform>(entity)) {
        interest::broadcast(packet, transform->chunk);
    }
    else {
        protocol::broadcast(packet, globals::host);
    }

    interest::forget(entity);
}

static void process_entity(entt::entity entity, DirtyMarker& dirty)
{
    static packet::Entity_Patch packet;

    packet.entity = entity;
    packet.components.clear();

    for(component_id_type id = 0; id < dirty.markers.size(); id += 1) {
        if(dirty.markers[id]) {
            packet::Entity_Patch::Component component;
            component.id = id;

            WriteBuffer buffer;
            component_map::encode_net(id, entity, buffer);
            component.data = std::move(buffer.take());

            packet.components.emplace_back(std::move(component));
        }
    }

    if(packet.components.size()) {
        auto except = globals::registry.all_of<SessionRef>(entity) ? globals::registry.get<SessionRef>(entity).ptr->peer : nullptr;

        if(auto transform = globals::registry.try_get<Transform>(entity)) {
            interest::broadcast(packet, transform->chunk, except);
        }
        else if(except) {
            protocol::broadcast(packet, globals::host, except);
        }
        else {
            protocol::broadcast(packet, globals::host);
        }

        std::ranges::fill(dirty.markers, false);
    }
}

void collector::init(void)
{
    globals::registry.on_construct<EntityClass>().connect<&on_create_entity>();
    globals::registry.on_destroy<EntityClass>().connect<&on_destroy_entity>();
}

void collector::fixed_update_late(void)
{
    auto view = globals::registry.view<DirtyMarker>();

    for(auto [entity, dirty] : view.each()) {
        process_entity(entity, dirty);
    }
}
