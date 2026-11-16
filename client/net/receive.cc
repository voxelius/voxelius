#include "client/pch.hh"

#include "client/net/receive.hh"

#include "core/msg/log.hh"

#include "shared/entity/class_registry.hh"
#include "shared/entity/component_map.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/packet_session.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/entity.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"
#include "client/net/session.hh"
#include "client/world/chunk_visibility.hh"

// Five seconds at the fixed rate; matches the server side log.
constexpr static std::uint64_t STATS_INTERVAL = 100;

static void on_entity_spawn(const packet::Entity_Spawn& packet)
{
    auto entity = utils::spawn(packet.class_id, packet.entity);

    if(!globals::registry.valid(entity)) {
        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC);
        return;
    }

    if(entity != packet.entity) {
        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC);
        return;
    }

    auto class_name = class_registry::name_of(packet.class_id);

    if(class_name.has_value()) {
        msg::log_info("entity spawn: id={} class={}", static_cast<std::uint64_t>(entity), class_name->full_string());
    }
    else {
        msg::log_info("entity spawn: id={} class=<null>", static_cast<std::uint64_t>(entity));
    }
}

static void on_entity_patch(const packet::Entity_Patch& packet)
{
    static ReadBuffer buffer;

    for(auto& it : packet.components) {
        buffer.reset(it.data);

        component_map::decode_net(it.id, packet.entity, buffer);
    }

    msg::log_info("entity patch: id={} components={}", static_cast<std::uint64_t>(packet.entity), packet.components.size());
}

static void on_entity_remove(const packet::Entity_Remove& packet)
{
    msg::log_info("entity remove: id={}", static_cast<std::uint64_t>(packet.entity));

    globals::registry.destroy(packet.entity);

    if(globals::player == packet.entity) {
        globals::player = entt::null;

        session::disconnect(packet::Session_Disconnect::ENTITY_ID_DESYNC); // We can't really recover from this
    }
}

static void on_entity_client(const packet::Entity_Client& packet)
{
    msg::log_info("assign entity {} as a local player", static_cast<std::uint64_t>(packet.entity));

    globals::player = packet.entity;

    session::notify_spawned();
}

static void on_world_blocks(const packet::World_Blocks& packet)
{
    chunk_visibility::note_received(packet.cpos);

    // A full chunk packet is authoritative: it is both the first load and
    // the resync the server sends to roll a stale client world back. Block
    // edits made after the packet was queued arrive as SetBlock on the same
    // reliable channel, so applying the whole chunk here cannot win over a
    // newer prediction.
    auto chunk = world::find_chunk(packet.cpos);

    if(chunk == nullptr) {
        chunk = world::create_chunk(packet.cpos);
    }

    chunk->set_blocks(packet.blocks);

    if(packet.light_valid) {
        chunk->set_light(packet.light);
    }
    else {
        chunk->clear_light();
    }

    globals::dispatcher.trigger(ChunkUpdateEvent(packet.cpos, chunk));
}

static void on_world_biomes(const packet::World_Biomes& packet)
{
    biome_map::insert(packet.realm, packet.cpos, packet.biomes);
}

static void on_world_light(const packet::World_Light& packet)
{
    for(const auto& entry : packet.entries) {
        auto chunk = world::find_chunk(entry.cpos);

        if(chunk == nullptr) {
            continue;
        }

        if(entry.full) {
            chunk->set_light(entry.light);
        }
        else {
            // A delta cannot seed a chunk the client was never given light
            // for; the server sends the whole array in that case.
            if(!chunk->has_light()) {
                continue;
            }

            for(std::size_t i = 0; i < entry.indices.size(); i += 1) {
                chunk->set_packed_light(entry.indices[i], entry.values[i]);
            }
        }

        globals::dispatcher.trigger(ChunkLightUpdateEvent(entry.cpos));
    }
}

static void on_world_set_block(const packet::World_SetBlock& packet)
{
    world::set_block(packet.bpos, packet.block);
}

static void on_world_timings(const packet::World_Timings& packet)
{
    globals::current_tick = packet.current_tick;

    static std::uint64_t last_tick;

    if(globals::current_tick - last_tick < STATS_INTERVAL) {
        return;
    }

    last_tick = globals::current_tick;

    auto stats = protocol::take_received_stats();

    msg::log_info("net recv: chunks={} biomes={} light={} setblock={} entities={} timings={} other={}", stats.chunks, stats.biomes,
        stats.light, stats.set_block, stats.entities, stats.timings, stats.other);
}

static void on_session_state(const SessionStateEvent& event)
{
    if(SESSION_DISCONNECTED == event.state()) {
        globals::player = entt::null;
    }
}

void receive::init(void)
{
    globals::dispatcher.sink<packet::Entity_Spawn>().connect<&on_entity_spawn>();
    globals::dispatcher.sink<packet::Entity_Patch>().connect<&on_entity_patch>();
    globals::dispatcher.sink<packet::Entity_Remove>().connect<&on_entity_remove>();
    globals::dispatcher.sink<packet::Entity_Client>().connect<&on_entity_client>();

    globals::dispatcher.sink<packet::World_Blocks>().connect<&on_world_blocks>();
    globals::dispatcher.sink<packet::World_Biomes>().connect<&on_world_biomes>();
    globals::dispatcher.sink<packet::World_Light>().connect<&on_world_light>();
    globals::dispatcher.sink<packet::World_SetBlock>().connect<&on_world_set_block>();
    globals::dispatcher.sink<packet::World_Timings>().connect<&on_world_timings>();

    globals::dispatcher.sink<SessionStateEvent>().connect<&on_session_state>();
}
