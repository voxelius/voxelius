#include "server/pch.hh"

#include "server/net/interest.hh"

#include "core/cvar/host.hh"

#include "shared/component/transform.hh"
#include "shared/entity/class.hh"
#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/globals.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/view.hh"
#include "shared/world/world.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"
#include "server/universe.hh"
#include "server/world/world.hh"

cvar::Number<unsigned> interest::view_distance { 8, 1, 32 };

static void send_full_snapshot(entt::registry& registry, ENetPeer* peer, entt::entity entity)
{
    auto& entity_class = registry.get<EntityClass>(entity);

    packet::Entity_Spawn spawn_packet {};
    spawn_packet.entity = entity;
    spawn_packet.class_id = entity_class.id;
    protocol::send(spawn_packet, peer);

    packet::Entity_Patch patch_packet {};
    patch_packet.entity = entity;

    class_registry::for_each(entity_class.id, [&](component_id_type id) {
        packet::Entity_Patch::Component component;
        component.id = id;

        WriteBuffer buffer;
        component_map::encode_net(registry, id, entity, buffer);
        component.data = std::move(buffer.take());

        patch_packet.components.emplace_back(std::move(component));
    });

    if(patch_packet.components.size()) {
        protocol::send(patch_packet, peer);
    }
}

static void update_visibility(void)
{
    ZoneScoped;

    for(const auto& world : universe::worlds()) {
        auto& registry = world->registry();
        auto entities = registry.view<EntityClass, Transform>();

        for(auto& session : sessions::all()) {
            if(session.world.get() != static_cast<World*>(world.get()) || session.peer == nullptr) {
                continue;
            }

            for(const auto [entity, cls, transform] : entities.each()) {
                auto in_box = session.view_box.contains(transform.chunk);
                auto is_known = session.known_entities.contains(entity);

                if(in_box && !is_known) {
                    send_full_snapshot(registry, session.peer, entity);
                    session.known_entities.insert(entity);
                }
                else if(!in_box && is_known) {
                    packet::Entity_Remove packet {};
                    packet.entity = entity;
                    protocol::send(packet, session.peer);
                    session.known_entities.erase(entity);
                }
            }
        }
    }
}

void interest::init(void)
{
    auto& sv_view_distance_binding = cvar::bind_var(view_distance, "sv_view_distance");
    sv_view_distance_binding.set_help("Server view distance, in chunks");
    sv_view_distance_binding.set_flags(FCVAR_ARCHIVE);
}

void interest::fixed_update_late(void)
{
    ZoneScoped;

    auto radius = static_cast<ChunkPos::value_type>(view_distance.value());

    for(auto& session : sessions::all()) {
        if(session.peer == nullptr || session.world == nullptr) {
            continue;
        }

        auto& registry = session.world->registry();

        if(registry.valid(session.player)) {
            auto& transform = registry.get<Transform>(session.player);
            session.view_box = utils::view_box(transform.chunk, radius);
        }
    }

    update_visibility();
}

template<typename T>
void interest::broadcast(const T& packet, const ChunkPos& origin, ENetPeer* except)
{
    for(const auto& session : sessions::all()) {
        if(session.peer == nullptr || session.peer == except) {
            continue;
        }

        if(session.view_box.contains(origin)) {
            protocol::send(packet, session.peer);
        }
    }
}

template void interest::broadcast<packet::World_SetBlock>(const packet::World_SetBlock& packet, const ChunkPos& origin, ENetPeer* except);
template void interest::broadcast<packet::World_Light>(const packet::World_Light& packet, const ChunkPos& origin, ENetPeer* except);
template void interest::broadcast<packet::Entity_Spawn>(const packet::Entity_Spawn& packet, const ChunkPos& origin, ENetPeer* except);
template void interest::broadcast<packet::Entity_Patch>(const packet::Entity_Patch& packet, const ChunkPos& origin, ENetPeer* except);
template void interest::broadcast<packet::Entity_Remove>(const packet::Entity_Remove& packet, const ChunkPos& origin, ENetPeer* except);

void interest::mark_known(entt::entity entity, const ChunkPos& chunk)
{
    for(auto& session : sessions::all()) {
        if(session.peer && session.view_box.contains(chunk)) {
            session.known_entities.insert(entity);
        }
    }
}

void interest::mark_known(entt::entity entity)
{
    for(auto& session : sessions::all()) {
        if(session.peer) {
            session.known_entities.insert(entity);
        }
    }
}

void interest::forget(entt::entity entity)
{
    for(auto& session : sessions::all()) {
        session.known_entities.erase(entity);
    }
}
